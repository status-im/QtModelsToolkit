#include <QSignalSpy>
#include <QTest>

#include <QIdentityProxyModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include <qtmodelstoolkit/reverseproxymodel.h>

#include <qtmodelstoolkit/testing/listmodelwrapper.h>
#include <qtmodelstoolkit/testing/modelsignalsspy.h>
#include <qtmodelstoolkit/testing/modeltestutils.h>
#include <qtmodelstoolkit/testing/persistentindexestester.h>
#include <qtmodelstoolkit/testing/testmodel.h>

using namespace qtmt;

namespace {

// A flat model whose notifications are driven directly from the test, for
// signal shapes the stock test models don't produce - a dataChanged spanning
// several rows in particular.
class DrivenModel : public QAbstractListModel
{
public:
    explicit DrivenModel(QStringList values)
        : m_values{std::move(values)} { }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_values.size());
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::UserRole, "name"}};
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid)
                || role != Qt::UserRole)
            return {};

        return m_values.at(index.row());
    }

    void changeRange(int first, int last, const QString& prefix)
    {
        for (int i = first; i <= last; ++i)
            m_values[i] = prefix + m_values.at(i);

        emit dataChanged(index(first, 0), index(last, 0), {Qt::UserRole});
    }

private:
    QStringList m_values;
};

QJsonArray toJsonArray(const char* json)
{
    return QJsonDocument::fromJson(json).array();
}

QJsonArray reversed(const QJsonArray& array)
{
    QJsonArray result;

    for (auto i = array.size() - 1; i >= 0; --i)
        result.append(array.at(i));

    return result;
}

QJsonObject row(const QString& name, int value)
{
    return QJsonObject{{"name", name}, {"value", value}};
}

} // unnamed namespace

class TestReverseProxyModel : public QObject
{
    Q_OBJECT

    static constexpr auto Source = R"([
        { "name": "A", "value": 1 },
        { "name": "B", "value": 2 },
        { "name": "C", "value": 3 },
        { "name": "D", "value": 4 },
        { "name": "E", "value": 5 }
    ])";

    static QModelIndex indexArg(const QList<QVariant>& args, int position)
    {
        return args.at(position).value<QModelIndex>();
    }

private slots:
    void initializationTest()
    {
        ReverseProxyModel model;

        QCOMPARE(model.sourceModel(), nullptr);
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.columnCount(), 0);
        QVERIFY(model.roleNames().isEmpty());

        QVERIFY(!model.index(0, 0).isValid());
        QVERIFY(!model.mapToSource({}).isValid());
        QVERIFY(!model.mapFromSource({}).isValid());
        QVERIFY(!model.parent({}).isValid());
    }

    void basicReversalTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);
        ListModelWrapper expected(engine, reversed(toJsonArray(Source)));

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        QCOMPARE(model.rowCount(), 5);
        QCOMPARE(model.columnCount(), 1);
        QCOMPARE(model.roleNames(), sourceModel.model()->roleNames());
        QVERIFY(isSame(&model, expected));
    }

    void emptySourceTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, "[]");

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.index(0, 0).isValid());
        QVERIFY(isSame(&model, sourceModel));
    }

    void singleRowSourceTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, R"([{ "name": "A" }])");

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        QVERIFY(isSame(&model, sourceModel));
    }

    void mappingTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");

        for (auto i = 0; i < model.rowCount(); ++i) {
            const auto proxyIndex = model.index(i, 0);

            QVERIFY(proxyIndex.isValid());
            QCOMPARE(proxyIndex.model(), &model);
            QVERIFY(!proxyIndex.parent().isValid());

            const auto sourceIndex = model.mapToSource(proxyIndex);

            QCOMPARE(sourceIndex.row(), model.rowCount() - 1 - i);
            QCOMPARE(sourceIndex.model(), sourceModel.model());

            // the round trip lands back where it started, and the data agrees
            QCOMPARE(model.mapFromSource(sourceIndex), proxyIndex);
            QCOMPARE(proxyIndex.data(nameRole), sourceIndex.data(nameRole));
        }

        // out of range, and anything but the root as a parent, yield nothing
        QVERIFY(!model.index(-1, 0).isValid());
        QVERIFY(!model.index(5, 0).isValid());
        QVERIFY(!model.index(0, 1).isValid());
        QVERIFY(!model.index(0, 0, model.index(1, 0)).isValid());
        QCOMPARE(model.rowCount(model.index(1, 0)), 0);
        QCOMPARE(model.columnCount(model.index(1, 0)), 0);

        QVERIFY(!model.mapToSource({}).isValid());
        QVERIFY(!model.mapFromSource({}).isValid());
    }

    void setSourceModelTest()
    {
        QQmlEngine engine;

        ListModelWrapper firstSource(engine, Source);
        ListModelWrapper secondSource(engine, R"([
            { "name": "X", "value": 9 },
            { "name": "Y", "value": 8 }
        ])");

        ReverseProxyModel model;

        {
            ModelSignalsSpy spy(&model);
            QSignalSpy sourceModelChangedSpy(
                        &model, &QAbstractProxyModel::sourceModelChanged);

            model.setSourceModel(firstSource);

            QCOMPARE(spy.count(), 2);
            QCOMPARE(spy.modelAboutToBeResetSpy.count(), 1);
            QCOMPARE(spy.modelResetSpy.count(), 1);
            QCOMPARE(sourceModelChangedSpy.count(), 1);
            QCOMPARE(model.sourceModel(), firstSource.model());
        }

        // setting the very same model again changes nothing, and says nothing
        {
            ModelSignalsSpy spy(&model);

            model.setSourceModel(firstSource);

            QCOMPARE(spy.count(), 0);
        }

        {
            ModelSignalsSpy spy(&model);

            model.setSourceModel(secondSource);

            QCOMPARE(spy.count(), 2);
            QCOMPARE(spy.modelResetSpy.count(), 1);

            ListModelWrapper expected(engine, R"([
                { "name": "Y", "value": 8 },
                { "name": "X", "value": 9 }
            ])");

            QVERIFY(isSame(&model, expected));
        }

        // the model the proxy no longer watches cannot reach it any more
        {
            ModelSignalsSpy spy(&model);

            firstSource.append(QJsonArray{row(QStringLiteral("Z"), 7)});

            QCOMPARE(spy.count(), 0);
        }

        {
            ModelSignalsSpy spy(&model);

            model.setSourceModel(nullptr);

            QCOMPARE(spy.count(), 2);
            QCOMPARE(spy.modelResetSpy.count(), 1);
            QCOMPARE(model.sourceModel(), nullptr);
            QCOMPARE(model.rowCount(), 0);
            QVERIFY(model.roleNames().isEmpty());
        }
    }

    void insertAtSourceFrontIsProxyAppendTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");

        // the last source row, which is the proxy's first one
        const QPersistentModelIndex firstRow(model.index(0, 0));

        ModelSignalsSpy spy(&model);

        sourceModel.insert(0, row(QStringLiteral("X"), 6));
        sourceJson.prepend(row(QStringLiteral("X"), 6));

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.rowsAboutToBeInsertedSpy.count(), 1);
        QCOMPARE(spy.rowsInsertedSpy.count(), 1);

        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(indexArg(args, 0), QModelIndex{});
        QCOMPARE(args.at(1).toInt(), 5);
        QCOMPARE(args.at(2).toInt(), 5);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));

        // nothing the proxy already showed moved
        QCOMPARE(firstRow.row(), 0);
        QCOMPARE(firstRow.data(nameRole).toString(), QStringLiteral("E"));
    }

    void insertAtSourceBackIsProxyPrependTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");
        const QPersistentModelIndex firstRow(model.index(0, 0));

        ModelSignalsSpy spy(&model);

        sourceModel.append(QJsonArray{row(QStringLiteral("X"), 6)});
        sourceJson.append(row(QStringLiteral("X"), 6));

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.rowsAboutToBeInsertedSpy.count(), 1);

        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(indexArg(args, 0), QModelIndex{});
        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 0);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));

        // everything shifted by one, and kept pointing at its own row
        QCOMPARE(firstRow.row(), 1);
        QCOMPARE(firstRow.data(nameRole).toString(), QStringLiteral("E"));
    }

    void insertInTheMiddleTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        ModelSignalsSpy spy(&model);

        sourceModel.insert(2, row(QStringLiteral("X"), 6));
        sourceJson.insert(2, row(QStringLiteral("X"), 6));

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(args.at(1).toInt(), 3);
        QCOMPARE(args.at(2).toInt(), 3);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));
    }

    void insertManyRowsTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        ModelSignalsSpy spy(&model);

        const QJsonArray inserted{row(QStringLiteral("X"), 6),
                                  row(QStringLiteral("Y"), 7),
                                  row(QStringLiteral("Z"), 8)};

        sourceModel.insert(1, inserted);

        for (auto i = inserted.size() - 1; i >= 0; --i)
            sourceJson.insert(1, inserted.at(i));

        QCOMPARE(spy.count(), 2);

        // source rows 1..3 of a model of eight: the block keeps its order, and
        // lands as proxy rows 4..6
        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(args.at(1).toInt(), 4);
        QCOMPARE(args.at(2).toInt(), 6);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));
    }

    void insertIntoEmptySourceTest()
    {
        TestModel sourceModel(QList<QString>{QStringLiteral("name")});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        QCOMPARE(model.rowCount(), 0);

        {
            ModelSignalsSpy spy(&model);

            sourceModel.append({QStringLiteral("A")});

            QCOMPARE(spy.count(), 2);

            const auto args = spy.rowsAboutToBeInsertedSpy.first();

            QCOMPARE(args.at(1).toInt(), 0);
            QCOMPARE(args.at(2).toInt(), 0);
        }

        {
            ModelSignalsSpy spy(&model);

            sourceModel.append({QStringLiteral("B")});

            QCOMPARE(spy.count(), 2);

            const auto args = spy.rowsAboutToBeInsertedSpy.first();

            QCOMPARE(args.at(1).toInt(), 0);
            QCOMPARE(args.at(2).toInt(), 0);
        }

        const auto nameRole = sourceModel.roleNames().key("name");

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.index(0, 0).data(nameRole).toString(),
                 QStringLiteral("B"));
        QCOMPARE(model.index(1, 0).data(nameRole).toString(),
                 QStringLiteral("A"));
    }

    // The first insertion into a model with no roles yet announces new role
    // names along with the rows, without a reset (QTBUG-57971).
    void insertDefiningRolesTest()
    {
        TestModel sourceModel;

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        QVERIFY(model.roleNames().isEmpty());

        ModelSignalsSpy spy(&model);

        sourceModel.appendAndInitRoles({{QStringLiteral("name"),
                                         {QStringLiteral("A"),
                                          QStringLiteral("B"),
                                          QStringLiteral("C")}}});

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 2);

        QCOMPARE(model.roleNames(), sourceModel.roleNames());

        const auto nameRole = sourceModel.roleNames().key("name");

        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.index(0, 0).data(nameRole).toString(),
                 QStringLiteral("C"));
        QCOMPARE(model.index(2, 0).data(nameRole).toString(),
                 QStringLiteral("A"));
    }

    void removeFromSourceFrontTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");
        const QPersistentModelIndex firstRow(model.index(0, 0));

        ModelSignalsSpy spy(&model);

        sourceModel.remove(0);
        sourceJson.removeAt(0);

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.rowsAboutToBeRemovedSpy.count(), 1);
        QCOMPARE(spy.rowsRemovedSpy.count(), 1);

        const auto args = spy.rowsAboutToBeRemovedSpy.first();

        QCOMPARE(indexArg(args, 0), QModelIndex{});
        QCOMPARE(args.at(1).toInt(), 4);
        QCOMPARE(args.at(2).toInt(), 4);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));

        QCOMPARE(firstRow.row(), 0);
        QCOMPARE(firstRow.data(nameRole).toString(), QStringLiteral("E"));
    }

    void removeFromSourceBackTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");

        // the second proxy row, "D", which the removal of "E" brings to front
        const QPersistentModelIndex secondRow(model.index(1, 0));

        ModelSignalsSpy spy(&model);

        sourceModel.remove(4);
        sourceJson.removeAt(4);

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeRemovedSpy.first();

        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 0);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));

        QCOMPARE(secondRow.row(), 0);
        QCOMPARE(secondRow.data(nameRole).toString(), QStringLiteral("D"));
    }

    void removeFromTheMiddleTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        ModelSignalsSpy spy(&model);

        sourceModel.remove(1, 2);
        sourceJson.removeAt(1);
        sourceJson.removeAt(1);

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeRemovedSpy.first();

        QCOMPARE(args.at(1).toInt(), 2);
        QCOMPARE(args.at(2).toInt(), 3);

        ListModelWrapper expected(engine, reversed(sourceJson));
        QVERIFY(isSame(&model, expected));
    }

    void removeAllRowsTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        ModelSignalsSpy spy(&model);

        sourceModel.remove(0, 5);

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeRemovedSpy.first();

        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 4);

        QCOMPARE(model.rowCount(), 0);
        QVERIFY(isSame(&model, sourceModel));
    }

    void moveDownTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        PersistentIndexesTester indexesTester(&model);
        ModelSignalsSpy spy(&model);

        // "A" moves down to source row 2: [B, C, A, D, E]
        sourceModel.move(0, 2, 1);

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.rowsAboutToBeMovedSpy.count(), 1);
        QCOMPARE(spy.rowsMovedSpy.count(), 1);

        const auto args = spy.rowsAboutToBeMovedSpy.first();

        QCOMPARE(indexArg(args, 0), QModelIndex{});
        QCOMPARE(args.at(1).toInt(), 4);
        QCOMPARE(args.at(2).toInt(), 4);
        QCOMPARE(indexArg(args, 3), QModelIndex{});
        QCOMPARE(args.at(4).toInt(), 2);

        ListModelWrapper expected(engine, R"([
            { "name": "E", "value": 5 },
            { "name": "D", "value": 4 },
            { "name": "A", "value": 1 },
            { "name": "C", "value": 3 },
            { "name": "B", "value": 2 }
        ])");

        QVERIFY(isSame(&model, expected));
        QVERIFY(indexesTester.compare());
    }

    void moveUpTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        PersistentIndexesTester indexesTester(&model);
        ModelSignalsSpy spy(&model);

        // "D" moves up to source row 1: [A, D, B, C, E]
        sourceModel.move(3, 1, 1);

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeMovedSpy.first();

        QCOMPARE(args.at(1).toInt(), 1);
        QCOMPARE(args.at(2).toInt(), 1);
        QCOMPARE(args.at(4).toInt(), 4);

        ListModelWrapper expected(engine, R"([
            { "name": "E", "value": 5 },
            { "name": "C", "value": 3 },
            { "name": "B", "value": 2 },
            { "name": "D", "value": 4 },
            { "name": "A", "value": 1 }
        ])");

        QVERIFY(isSame(&model, expected));
        QVERIFY(indexesTester.compare());
    }

    void moveBlockTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        PersistentIndexesTester indexesTester(&model);
        ModelSignalsSpy spy(&model);

        // "A", "B" move down to source row 2: [C, D, A, B, E]
        sourceModel.move(0, 2, 2);

        QCOMPARE(spy.count(), 2);

        const auto args = spy.rowsAboutToBeMovedSpy.first();

        QCOMPARE(args.at(1).toInt(), 3);
        QCOMPARE(args.at(2).toInt(), 4);
        QCOMPARE(args.at(4).toInt(), 1);

        ListModelWrapper expected(engine, R"([
            { "name": "E", "value": 5 },
            { "name": "B", "value": 2 },
            { "name": "A", "value": 1 },
            { "name": "D", "value": 4 },
            { "name": "C", "value": 3 }
        ])");

        QVERIFY(isSame(&model, expected));
        QVERIFY(indexesTester.compare());
    }

    void dataChangedTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");

        ModelSignalsSpy spy(&model);

        sourceModel.setProperty(1, QStringLiteral("name"),
                                QStringLiteral("Z"));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.dataChangedSpy.count(), 1);

        const auto args = spy.dataChangedSpy.first();

        QCOMPARE(indexArg(args, 0).row(), 3);
        QCOMPARE(indexArg(args, 1).row(), 3);
        QCOMPARE(indexArg(args, 0).model(), &model);

        auto changedRow = sourceJson.at(1).toObject();
        changedRow["name"] = QStringLiteral("Z");
        sourceJson.replace(1, changedRow);

        ListModelWrapper expected(engine, reversed(sourceJson));

        QVERIFY(isSame(&model, expected));
        QCOMPARE(model.index(3, 0).data(nameRole).toString(),
                 QStringLiteral("Z"));
    }

    void dataChangedRangeTest()
    {
        DrivenModel sourceModel({QStringLiteral("A"), QStringLiteral("B"),
                                 QStringLiteral("C"), QStringLiteral("D"),
                                 QStringLiteral("E")});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        ModelSignalsSpy spy(&model);

        sourceModel.changeRange(1, 3, QStringLiteral("x"));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.dataChangedSpy.count(), 1);

        // source rows 1..3 are proxy rows 1..3 turned inside out
        const auto args = spy.dataChangedSpy.first();

        QCOMPARE(indexArg(args, 0).row(), 1);
        QCOMPARE(indexArg(args, 1).row(), 3);
        QCOMPARE(args.at(2).value<QVector<int>>(),
                 QVector<int>{Qt::UserRole});

        QCOMPARE(model.index(1, 0).data(Qt::UserRole).toString(),
                 QStringLiteral("xD"));
        QCOMPARE(model.index(3, 0).data(Qt::UserRole).toString(),
                 QStringLiteral("xB"));
        QCOMPARE(model.index(0, 0).data(Qt::UserRole).toString(),
                 QStringLiteral("E"));
    }

    void resetTest()
    {
        TestModel sourceModel({{QStringLiteral("name"),
                                {QStringLiteral("A"), QStringLiteral("B"),
                                 QStringLiteral("C")}}});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        const auto nameRole = sourceModel.roleNames().key("name");
        const QPersistentModelIndex firstRow(model.index(0, 0));

        ModelSignalsSpy spy(&model);

        sourceModel.reset({{QStringLiteral("name"),
                            {QStringLiteral("X"), QStringLiteral("Y")}}});

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(spy.modelResetSpy.count(), 1);

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.index(0, 0).data(nameRole).toString(),
                 QStringLiteral("Y"));
        QCOMPARE(model.index(1, 0).data(nameRole).toString(),
                 QStringLiteral("X"));

        // a reset is the one change that invalidates what was held
        QVERIFY(!firstRow.isValid());
    }

    void resetAndClearTest()
    {
        TestModel sourceModel({{QStringLiteral("name"),
                                {QStringLiteral("A"), QStringLiteral("B")}}});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        ModelSignalsSpy spy(&model);

        sourceModel.resetAndClear();

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.modelResetSpy.count(), 1);
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.roleNames(), sourceModel.roleNames());
    }

    void layoutChangedTest()
    {
        TestModel sourceModel({{QStringLiteral("name"),
                                {QStringLiteral("A"), QStringLiteral("B"),
                                 QStringLiteral("C"), QStringLiteral("D"),
                                 QStringLiteral("E")}}});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        const auto nameRole = sourceModel.roleNames().key("name");

        PersistentIndexesTester indexesTester(&model);
        ModelSignalsSpy spy(&model);

        // the source inverts itself, so the proxy ends up in the source's
        // original order
        sourceModel.invert();

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.layoutAboutToBeChangedSpy.count(), 1);
        QCOMPARE(spy.layoutChangedSpy.count(), 1);

        QCOMPARE(model.index(0, 0).data(nameRole).toString(),
                 QStringLiteral("A"));
        QCOMPARE(model.index(4, 0).data(nameRole).toString(),
                 QStringLiteral("E"));

        QVERIFY(indexesTester.compare());
    }

    // A source may announce rows going away through a layout change alone -
    // SFPM does exactly that while filtering for the first time.
    void layoutChangedRemovingRowsTest()
    {
        TestModel sourceModel({{QStringLiteral("name"),
                                {QStringLiteral("A"), QStringLiteral("B"),
                                 QStringLiteral("C"), QStringLiteral("D"),
                                 QStringLiteral("E")}}});

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        const auto nameRole = sourceModel.roleNames().key("name");

        QList<QPersistentModelIndex> held;

        for (auto i = 0; i < model.rowCount(); ++i)
            held.append(QPersistentModelIndex(model.index(i, 0)));

        ModelSignalsSpy spy(&model);

        // leaves the source holding "B" and "D"
        sourceModel.removeEverySecond();

        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.layoutChangedSpy.count(), 1);

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.index(0, 0).data(nameRole).toString(),
                 QStringLiteral("D"));
        QCOMPARE(model.index(1, 0).data(nameRole).toString(),
                 QStringLiteral("B"));

        // what survived followed its row; what did not went invalid
        QVERIFY(!held.at(0).isValid());          // was "E"
        QCOMPARE(held.at(1).row(), 0);           // was "D"
        QVERIFY(!held.at(2).isValid());          // was "C"
        QCOMPARE(held.at(3).row(), 1);           // was "B"
        QVERIFY(!held.at(4).isValid());          // was "A"

        QCOMPARE(held.at(1).data(nameRole).toString(), QStringLiteral("D"));
        QCOMPARE(held.at(3).data(nameRole).toString(), QStringLiteral("B"));
    }

    void persistentIndexesAcrossInsertionsTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        const auto nameRole = sourceModel.role("name");

        QList<QPersistentModelIndex> held;

        for (auto i = 0; i < model.rowCount(); ++i)
            held.append(QPersistentModelIndex(model.index(i, 0)));

        // one insertion at each end of the source, and one inside it
        sourceModel.append(QJsonArray{row(QStringLiteral("X"), 6)});
        sourceModel.insert(0, row(QStringLiteral("Y"), 7));
        sourceModel.insert(3, row(QStringLiteral("Z"), 8));

        const QStringList expectedNames{QStringLiteral("E"),
                                        QStringLiteral("D"),
                                        QStringLiteral("C"),
                                        QStringLiteral("B"),
                                        QStringLiteral("A")};

        for (auto i = 0; i < held.size(); ++i) {
            QVERIFY(held.at(i).isValid());
            QCOMPARE(held.at(i).data(nameRole).toString(),
                     expectedNames.at(i));
        }
    }

    void doubleReversalIsIdentityTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel reversedOnce;
        reversedOnce.setSourceModel(sourceModel);

        ReverseProxyModel reversedTwice;
        reversedTwice.setSourceModel(&reversedOnce);

        QVERIFY(isSame(&reversedTwice, sourceModel));

        ModelSignalsSpy spy(&reversedTwice);

        sourceModel.insert(0, row(QStringLiteral("X"), 6));
        sourceJson.prepend(row(QStringLiteral("X"), 6));

        QCOMPARE(spy.count(), 2);

        // a front insertion stays a front insertion after two reversals
        const auto args = spy.rowsAboutToBeInsertedSpy.first();

        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 0);

        ListModelWrapper expected(engine, sourceJson);

        QVERIFY(isSame(&reversedTwice, expected));
        QVERIFY(isSame(&reversedTwice, sourceModel));
    }

    // Whatever the proxy announces has to satisfy a proxy stacked on top of it;
    // QSortFilterProxyModel is strict about the signals it is fed.
    void downstreamProxyTest()
    {
        QQmlEngine engine;

        auto sourceJson = toJsonArray(Source);
        ListModelWrapper sourceModel(engine, sourceJson);

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        QSortFilterProxyModel downstream;
        downstream.setSourceModel(&model);

        // moves are the one change QSortFilterProxyModel does not pass on as
        // such, so a proxy that does is stacked up as well
        QIdentityProxyModel identity;
        identity.setSourceModel(&model);

        QVERIFY(isSame(&downstream, &model));
        QVERIFY(isSame(&identity, &model));

        ModelSignalsSpy spy(&downstream);
        ModelSignalsSpy identitySpy(&identity);

        sourceModel.insert(0, row(QStringLiteral("X"), 6));
        sourceJson.prepend(row(QStringLiteral("X"), 6));

        QCOMPARE(spy.rowsInsertedSpy.count(), 1);
        QCOMPARE(identitySpy.rowsInsertedSpy.count(), 1);

        sourceModel.append(QJsonArray{row(QStringLiteral("Y"), 7)});
        sourceJson.append(row(QStringLiteral("Y"), 7));

        QCOMPARE(spy.rowsInsertedSpy.count(), 2);
        QCOMPARE(identitySpy.rowsInsertedSpy.count(), 2);

        sourceModel.remove(2);
        sourceJson.removeAt(2);

        QCOMPARE(spy.rowsRemovedSpy.count(), 1);
        QCOMPARE(identitySpy.rowsRemovedSpy.count(), 1);

        ListModelWrapper expected(engine, reversed(sourceJson));

        QVERIFY(isSame(&downstream, expected));
        QVERIFY(isSame(&identity, expected));

        sourceModel.move(0, 3, 2);

        QCOMPARE(identitySpy.rowsMovedSpy.count(), 1);

        // the json no longer tracks the source, so the proxies are compared
        // against the one they wrap from here on
        QVERIFY(isSame(&downstream, &model));
        QVERIFY(isSame(&identity, &model));

        QCOMPARE(downstream.rowCount(), 6);

        // nothing along the way was given up on and re-read wholesale
        QCOMPARE(spy.modelResetSpy.count(), 0);
        QCOMPARE(identitySpy.modelResetSpy.count(), 0);
    }

    void sourceModelDestroyedTest()
    {
        auto* sourceModel = new TestModel({{QStringLiteral("name"),
                                            {QStringLiteral("A"),
                                             QStringLiteral("B")}}});

        ReverseProxyModel model;
        model.setSourceModel(sourceModel);

        QCOMPARE(model.rowCount(), 2);

        const QPersistentModelIndex firstRow(model.index(0, 0));

        delete sourceModel;

        QCOMPARE(model.sourceModel(), nullptr);
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.roleNames().isEmpty());
        QVERIFY(!firstRow.isValid());
        QVERIFY(!model.index(0, 0).isValid());
    }

    void nestedChangesAreRejectedTest()
    {
        QStandardItemModel sourceModel;

        auto* parentItem = new QStandardItem(QStringLiteral("A"));
        sourceModel.appendRow(parentItem);

        ReverseProxyModel model;
        model.setSourceModel(&sourceModel);

        QCOMPARE(model.rowCount(), 1);

        ModelSignalsSpy spy(&model);

        QTest::ignoreMessage(
                    QtWarningMsg,
                    "ReverseProxyModel: only flat models are supported");

        parentItem->appendRow(new QStandardItem(QStringLiteral("A1")));

        // the change happened below the root, so the flat proxy says nothing
        // about it - and, in particular, does not mistranslate it
        QCOMPARE(spy.count(), 0);
        QCOMPARE(model.rowCount(), 1);
    }

    void qmlInstantiationTest()
    {
        QQmlEngine engine;

        ListModelWrapper sourceModel(engine, Source);
        ListModelWrapper expected(engine, reversed(toJsonArray(Source)));

        QQmlComponent component(&engine);

        component.setData(R"(
            import QtQml 2.15
            import QtModelsToolkit 1.0

            ReverseProxyModel {
            }
        )", QUrl());

        const std::unique_ptr<QObject> object(component.create());

        QVERIFY2(object != nullptr, qPrintable(component.errorString()));

        auto* model = qobject_cast<ReverseProxyModel*>(object.get());

        QVERIFY(model != nullptr);

        QVERIFY(object->setProperty(
                    "sourceModel",
                    QVariant::fromValue(sourceModel.model())));

        QCOMPARE(model->sourceModel(), sourceModel.model());
        QVERIFY(isSame(model, expected));
    }
};

QTEST_MAIN(TestReverseProxyModel)
#include "tst_ReverseProxyModel.moc"

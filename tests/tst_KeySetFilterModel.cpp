#include <QAbstractItemModelTester>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>

#include <functional>
#include <memory>

#include <qtmodelstoolkit/keysetfiltermodel.h>
#include <qtmodelstoolkit/testing/testmodel.h>

using namespace qtmt;

namespace {

QStringList keys(const QAbstractItemModel& model, const QByteArray& role = "key")
{
    const auto roleId = model.roleNames().key(role, -1);
    QStringList result;

    for (int row = 0; row < model.rowCount(); row++)
        result << model.data(model.index(row, 0), roleId).toString();

    return result;
}

QList<int> sourceRows(const QAbstractProxyModel& model)
{
    QList<int> result;

    for (int row = 0; row < model.rowCount(); row++)
        result << model.mapToSource(model.index(row, 0)).row();

    return result;
}

// Mimics QML ListModel's setProperty: setting a value of an unknown role adds
// that role and is notified with dataChanged only.
class GrowingRolesModel : public QAbstractListModel
{
public:
    explicit GrowingRolesModel(int count)
        : m_rows(count)
    {
    }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return m_roles;
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        return m_rows.at(index.row()).value(role);
    }

    void setProperty(int row, const QByteArray& name, const QVariant& value)
    {
        auto role = m_roles.key(name, -1);

        if (role == -1) {
            role = Qt::UserRole + m_roles.size();
            m_roles.insert(role, name);
        }

        m_rows[row].insert(role, value);
        emit dataChanged(index(row), index(row), { role });
    }

private:
    QVector<QHash<int, QVariant>> m_rows;
    QHash<int, QByteArray> m_roles;
};

} // unnamed namespace

class TestKeySetFilterModel : public QObject
{
    Q_OBJECT

    std::unique_ptr<TestModel> m_source;
    std::unique_ptr<KeySetFilterModel> m_model;
    std::unique_ptr<QAbstractItemModelTester> m_tester;

private slots:
    void init()
    {
        m_source = std::make_unique<TestModel>(QList<QPair<QString, QVariantList>> {
            { "key", { "a", "b", "c", "d" }},
            { "name", { "A", "B", "C", "D" }}
        });

        m_model = std::make_unique<KeySetFilterModel>();
        m_model->setKeys({ "b", "d" });
        m_model->setSourceModel(m_source.get());

        m_tester = std::make_unique<QAbstractItemModelTester>(
                    m_model.get(), QAbstractItemModelTester::FailureReportingMode::QtTest);
    }

    void cleanup()
    {
        m_tester.reset();
        m_model.reset();
        m_source.reset();
    }

    void defaultKeyRoleName()
    {
        KeySetFilterModel model;
        QCOMPARE(model.keyRoleName(), QString("key"));
        QVERIFY(model.keys().isEmpty());
        QCOMPARE(model.invert(), false);
    }

    void filtersByKeys()
    {
        QCOMPARE(m_model->rowCount(), 2);
        QCOMPARE(keys(*m_model), QStringList({ "b", "d" }));
        QCOMPARE(keys(*m_model, "name"), QStringList({ "B", "D" }));
    }

    void keysAbsentFromSourceAreIgnored()
    {
        m_model->setKeys({ "x", "c", "y" });
        QCOMPARE(keys(*m_model), QStringList({ "c" }));
    }

    void emptySetGivesNoRows()
    {
        m_model->setKeys({});
        QCOMPARE(m_model->rowCount(), 0);

        KeySetFilterModel model;
        model.setSourceModel(m_source.get());
        QCOMPARE(model.rowCount(), 0);
    }

    void swappingSetRefiltersAndNotifiesOnce()
    {
        QSignalSpy keysSpy(m_model.get(), &KeySetFilterModel::keysChanged);

        m_model->setKeys({ "a", "c", "d" });

        QCOMPARE(keysSpy.count(), 1);
        QCOMPARE(m_model->keys(), QStringList({ "a", "c", "d" }));
        QCOMPARE(keys(*m_model), QStringList({ "a", "c", "d" }));

        // same set in a different order: nothing changes
        m_model->setKeys({ "d", "a", "c" });
        QCOMPARE(keysSpy.count(), 1);
        QCOMPARE(keys(*m_model), QStringList({ "a", "c", "d" }));
    }

    void invert()
    {
        QSignalSpy spy(m_model.get(), &KeySetFilterModel::invertChanged);

        m_model->setInvert(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(keys(*m_model), QStringList({ "a", "c" }));

        m_model->setInvert(true);
        QCOMPARE(spy.count(), 1);

        m_model->setKeys({});
        QCOMPARE(keys(*m_model), QStringList({ "a", "b", "c", "d" }));
    }

    void sourceInsertion()
    {
        m_source->insert(0, { "c2", "C2" });
        m_source->append({ "e", "E" });
        QCOMPARE(keys(*m_model), QStringList({ "b", "d" }));

        m_model->setKeys({ "b", "d", "e" });
        m_source->insert(1, { "x", "X" });
        m_source->append({ "e", "E2" });
        QCOMPARE(keys(*m_model), QStringList({ "b", "d", "e", "e" }));
        QCOMPARE(keys(*m_model, "name"), QStringList({ "B", "D", "E", "E2" }));
    }

    void sourceRemoval()
    {
        m_source->remove(1);
        QCOMPARE(keys(*m_model), QStringList({ "d" }));

        m_source->remove(0);
        QCOMPARE(keys(*m_model), QStringList({ "d" }));
    }

    void sourceKeyChange()
    {
        const auto keyRole = m_source->roleNames().key("key");
        const auto nameRole = m_source->roleNames().key("name");

        // key leaves the set
        m_source->update(1, keyRole, "x");
        QCOMPARE(keys(*m_model), QStringList({ "d" }));

        // key enters the set
        m_source->update(0, keyRole, "b");
        QCOMPARE(keys(*m_model), QStringList({ "b", "d" }));

        // other role of an accepted row is forwarded
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);
        m_source->update(3, nameRole, "D2");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(keys(*m_model, "name"), QStringList({ "A", "D2" }));
    }

    void sourceReset()
    {
        m_source->reset({
            { "key", { "d", "e", "b", "f" }},
            { "name", { "D", "E", "B", "F" }}
        });
        QCOMPARE(keys(*m_model), QStringList({ "d", "b" }));

        m_source->resetAndClear();
        QCOMPARE(m_model->rowCount(), 0);
    }

    void sourceResetWithDifferentRoles()
    {
        // key role moves to a different role id
        m_source->reset({
            { "name", { "A", "B" }},
            { "key", { "a", "b" }}
        });
        QCOMPARE(keys(*m_model), QStringList({ "b" }));

        // key role gone
        m_source->reset({ { "name", { "A", "B" }} });
        QCOMPARE(m_model->rowCount(), 0);
    }

    void sourceResetWithDifferentRolesReadMidReset()
    {
        // no tester: the proxy mapping is built first while the reset is
        // being notified, with the source still having the old roles
        TestModel source(QList<QPair<QString, QVariantList>> {
            { "key", { "a", "b", "c" }},
            { "name", { "A", "B", "C" }}
        });
        KeySetFilterModel model;
        model.setKeys({ "b" });
        model.setSourceModel(&source);

        connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model,
                [&model] { model.rowCount(); });

        // key role moves to a different role id
        source.reset({
            { "name", { "A", "B", "C" }},
            { "key", { "c", "b", "a" }}
        });

        QCOMPARE(sourceRows(model), QList<int>({ 1 }));
    }

    void sourceModelChangeWithDifferentRolesReadMidReset()
    {
        TestModel source(QList<QPair<QString, QVariantList>> {
            { "key", { "a", "b", "c" }},
            { "name", { "A", "B", "C" }}
        });
        TestModel other(QList<QPair<QString, QVariantList>> {
            { "name", { "A", "B", "C" }},
            { "key", { "c", "b", "a" }}
        });
        KeySetFilterModel model;
        model.setKeys({ "b" });
        model.setSourceModel(&source);

        connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model,
                [&model] { model.rowCount(); });

        model.setSourceModel(&other);

        QCOMPARE(sourceRows(model), QList<int>({ 1 }));
    }

    void keyRoleAddedByDataChange()
    {
        GrowingRolesModel source(3);
        source.setProperty(0, "name", "A");
        source.setProperty(1, "name", "B");
        source.setProperty(2, "name", "C");

        KeySetFilterModel model;
        model.setKeys({ "b", "c" });
        model.setSourceModel(&source);
        QAbstractItemModelTester tester(
                    &model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        QCOMPARE(model.rowCount(), 0);

        source.setProperty(1, "key", "b");
        QCOMPARE(sourceRows(model), QList<int>({ 1 }));

        source.setProperty(2, "key", "c");
        QCOMPARE(sourceRows(model), QList<int>({ 1, 2 }));
    }

    void keyRoleAddedByDataChangeInverted()
    {
        GrowingRolesModel source(3);
        source.setProperty(0, "name", "A");
        source.setProperty(1, "name", "B");
        source.setProperty(2, "name", "C");

        KeySetFilterModel model;
        model.setKeys({ "b" });
        model.setInvert(true);
        model.setSourceModel(&source);
        QAbstractItemModelTester tester(
                    &model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        QCOMPARE(model.rowCount(), 3);

        source.setProperty(1, "key", "b");
        QCOMPARE(sourceRows(model), QList<int>({ 0, 2 }));
    }

    void missingRole()
    {
        m_model->setKeyRoleName("absent");
        QCOMPARE(m_model->rowCount(), 0);

        m_model->setKeyRoleName("name");
        QCOMPARE(keys(*m_model), QStringList());

        m_model->setKeys({ "A", "C" });
        QCOMPARE(keys(*m_model, "name"), QStringList({ "A", "C" }));
    }

    void keyRoleNameChange()
    {
        QSignalSpy spy(m_model.get(), &KeySetFilterModel::keyRoleNameChanged);

        m_model->setKeys({ "B" });
        m_model->setKeyRoleName("name");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(keys(*m_model, "name"), QStringList({ "B" }));

        m_model->setKeyRoleName("name");
        QCOMPARE(spy.count(), 1);
    }

    void rolesInitializedOnFirstInsertion()
    {
        TestModel source;
        KeySetFilterModel model;
        model.setKeys({ "b" });
        model.setSourceModel(&source);

        QCOMPARE(model.rowCount(), 0);

        source.appendAndInitRoles({ { "key", { "a", "b", "c" }} });

        // checked via source rows: on Qt 5 the proxy's role names stay empty
        // after roles are defined this way
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.mapToSource(model.index(0, 0)).row(), 1);
    }

    void sourceModelChange()
    {
        TestModel other(QList<QPair<QString, QVariantList>> {
            { "id", { "1", "2", "3" }},
            { "key", { "d", "b", "z" }}
        });

        m_model->setSourceModel(&other);
        QCOMPARE(keys(*m_model), QStringList({ "d", "b" }));

        m_model->setSourceModel(nullptr);
        QCOMPARE(m_model->rowCount(), 0);
    }

    void largeSourceFilteredOnce()
    {
        constexpr int count = 5000;

        QVariantList keyValues;
        QVariantList names;
        QStringList set;
        keyValues.reserve(count);
        names.reserve(count);

        for (int i = 0; i < count; i++) {
            const auto key = QStringLiteral("0x%1").arg(i, 130, 16, QChar('0'));
            keyValues << key;
            names << QStringLiteral("member %1").arg(i);

            if (i % 2 == 0)
                set << key;
        }

        TestModel source(QList<QPair<QString, QVariantList>> {
            { "key", keyValues }, { "name", names }
        });
        KeySetFilterModel model;
        model.setSourceModel(&source);

        // forces the proxy mapping to be built for the empty set first
        QCOMPARE(model.rowCount(), 0);

        QElapsedTimer timer;
        timer.start();
        model.setKeys(set);
        const auto rows = model.rowCount();
        const auto elapsed = timer.nsecsElapsed();

        QCOMPARE(rows, count / 2);
        qInfo("5k rows, 2.5k keys, filtered once: %.3f ms", elapsed / 1e6);
    }

    void largeInterleavedSwapResets()
    {
        // two channels sharing every third member, the others interleaved:
        // incrementally it's one rowsRemoved/rowsInserted per changed row
        const auto result = swap(5000, [](int i) { return i % 3 != 2; },
                                 [](int i) { return i % 3 != 1; });

        qInfo("5k rows, interleaved swap: %d removed, %d inserted, %d reset, "
              "%.3f ms", result.removed, result.inserted, result.resets,
              result.elapsed / 1e6);

        QCOMPARE(result.resets, 1);
        QCOMPARE(result.removed, 0);
        QCOMPARE(result.inserted, 0);
    }

    void smallSwapIsIncremental()
    {
        // 100 keys changed out of 5k rows, below 1/8 of the source
        const auto result = swap(5000, [](int i) { return i < 2000; },
                                 [](int i) { return i < 1950 || (i >= 2000 && i < 2050); });

        QCOMPARE(result.resets, 0);
        QCOMPARE(result.removed, 1);
        QCOMPARE(result.inserted, 1);
    }

    void swapResetThreshold()
    {
        // 64 keys changed: at least 1/8 of 100 rows
        auto result = swap(100, [](int i) { return i < 32; },
                           [](int i) { return i >= 32 && i < 64; });

        QCOMPARE(result.resets, 1);
        QCOMPARE(result.removed, 0);
        QCOMPARE(result.inserted, 0);

        // 63 keys changed, below the minimum
        result = swap(100, [](int i) { return i < 32; },
                      [](int i) { return i >= 32 && i < 63; });

        QCOMPARE(result.resets, 0);
        QCOMPARE(result.removed, 1);
        QCOMPARE(result.inserted, 1);

        // 64 keys changed, below 1/8 of 1000 rows
        result = swap(1000, [](int i) { return i < 32; },
                      [](int i) { return i >= 32 && i < 64; });

        QCOMPARE(result.resets, 0);
        QCOMPARE(result.removed, 1);
        QCOMPARE(result.inserted, 1);
    }

private:
    struct SwapResult {
        int removed = 0;
        int inserted = 0;
        int resets = 0;
        qint64 elapsed = 0;
    };

    // Swaps the keys of a source of count rows from rows matching inA to rows
    // matching inB, verifying the resulting rows through a downstream proxy.
    static SwapResult swap(int count, const std::function<bool(int)>& inA,
                           const std::function<bool(int)>& inB)
    {
        QVariantList keyValues;
        QStringList setA;
        QStringList setB;
        QList<int> rowsB;

        for (int i = 0; i < count; i++) {
            const auto key = QStringLiteral("0x%1").arg(i, 130, 16, QChar('0'));
            keyValues << key;

            if (inA(i))
                setA << key;
            if (inB(i)) {
                setB << key;
                rowsB << i;
            }
        }

        TestModel source(QList<QPair<QString, QVariantList>> {{ "key", keyValues }});
        KeySetFilterModel model;
        model.setKeys(setA);
        model.setSourceModel(&source);

        // downstream proxy, as views and further proxies follow the signals
        QSortFilterProxyModel downstream;
        downstream.setSourceModel(&model);
        downstream.rowCount();

        QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
        QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

        QElapsedTimer timer;
        timer.start();
        model.setKeys(setB);
        const auto rows = downstream.rowCount();

        SwapResult result;
        result.elapsed = timer.nsecsElapsed();
        result.removed = removedSpy.count();
        result.inserted = insertedSpy.count();
        result.resets = resetSpy.count();

        if (rows != rowsB.size() || sourceRows(model) != rowsB)
            qFatal("swap: wrong rows after the swap");

        return result;
    }
};

QTEST_MAIN(TestKeySetFilterModel)
#include "tst_KeySetFilterModel.moc"

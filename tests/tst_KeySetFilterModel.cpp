#include <QAbstractItemModelTester>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>

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

        QCOMPARE(keys(model), QStringList({ "b" }));
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
};

QTEST_MAIN(TestKeySetFilterModel)
#include "tst_KeySetFilterModel.moc"

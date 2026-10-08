#include <QAbstractItemModelTester>
#include <QIdentityProxyModel>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

#include <memory>

#include <qtmodelstoolkit/rolesoverlaymodel.h>
#include <qtmodelstoolkit/testing/testmodel.h>

using namespace qtmt;

namespace {

int roleId(const QAbstractItemModel& model, const QByteArray& name)
{
    return model.roleNames().key(name, -1);
}

QVariant value(const QAbstractItemModel& model, int row, const QByteArray& role)
{
    return model.data(model.index(row, 0), roleId(model, role));
}

// counts reads of a single role, to check the per-update cost of the overlay
class RoleReadsCounter : public QIdentityProxyModel
{
public:
    int role = -1;
    mutable int reads = 0;

    QVariant data(const QModelIndex& index, int r) const override
    {
        if (r == role)
            ++reads;
        return QIdentityProxyModel::data(index, r);
    }
};

} // unnamed namespace

class TestRolesOverlayModel : public QObject
{
    Q_OBJECT

    std::unique_ptr<TestModel> m_source;
    std::unique_ptr<RolesOverlayModel> m_model;
    std::unique_ptr<QAbstractItemModelTester> m_tester;

private slots:
    void init()
    {
        m_source = std::make_unique<TestModel>(QList<QPair<QString, QVariantList>> {
            { "key", { "a", "b", "c" }},
            { "name", { "A", "B", "C" }}
        });

        m_model = std::make_unique<RolesOverlayModel>();
        m_model->setKeyRole("key");
        m_model->setDefaults({ { "pinned", false }, { "timestamp", 0 } });
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

    void addsOverlayRolesWithDefaults()
    {
        const auto roles = m_model->roleNames();
        QCOMPARE(roles.size(), 4);
        QVERIFY(roles.values().contains("pinned"));
        QVERIFY(roles.values().contains("timestamp"));

        for (int row = 0; row < 3; row++) {
            QCOMPARE(value(*m_model, row, "pinned"), QVariant(false));
            QCOMPARE(value(*m_model, row, "timestamp"), QVariant(0));
        }
        QCOMPARE(value(*m_model, 1, "name"), QVariant("B"));
    }

    void setByKeyNotifiesOnlyMatchingRow()
    {
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);

        m_model->set("b", "pinned", true);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toModelIndex().row(), 1);
        QCOMPARE(spy.at(0).at(1).toModelIndex().row(), 1);
        QCOMPARE(spy.at(0).at(2).value<QVector<int>>(),
                 QVector<int>{ roleId(*m_model, "pinned") });

        QCOMPARE(value(*m_model, 0, "pinned"), QVariant(false));
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(true));
        QCOMPARE(m_model->get("b", "pinned"), QVariant(true));

        // same value again: no notification
        m_model->set("b", "pinned", true);
        QCOMPARE(spy.count(), 1);
    }

    void setForAbsentKeyIsIgnored()
    {
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);

        m_model->set("absent", "pinned", true);

        QCOMPARE(spy.count(), 0);
        QVERIFY(m_model->entries().isEmpty());
        QCOMPARE(m_model->get("absent", "pinned"), QVariant(false));
    }

    void setUnknownRoleIsIgnored()
    {
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("has no default value"));

        m_model->set("b", "unknown", 1);

        QCOMPARE(spy.count(), 0);
        QVERIFY(m_model->entries().isEmpty());
    }

    void valuesFollowRows()
    {
        m_model->set("b", "timestamp", 1000);

        m_source->insert(0, { "z", "Z" });
        QCOMPARE(value(*m_model, 0, "timestamp"), QVariant(0));
        QCOMPARE(value(*m_model, 2, "timestamp"), QVariant(1000));

        m_source->invert(); // layout change
        QCOMPARE(value(*m_model, 1, "key"), QVariant("b"));
        QCOMPARE(value(*m_model, 1, "timestamp"), QVariant(1000));

        m_source->reset(); // same content
        QCOMPARE(value(*m_model, 1, "timestamp"), QVariant(1000));
    }

    void valuesOfRemovedRowsAreDropped()
    {
        m_model->set("b", "pinned", true);
        m_model->set("c", "pinned", true);

        m_source->remove(1);
        QCOMPARE(m_model->entries().size(), 1);

        m_source->append({ "b", "B" });
        QCOMPARE(value(*m_model, 2, "key"), QVariant("b"));
        QCOMPARE(value(*m_model, 2, "pinned"), QVariant(false));
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(true));
    }

    void valuesOfKeysGoneAfterLayoutChangeOrResetAreDropped()
    {
        m_model->set("a", "pinned", true);
        m_model->set("b", "pinned", true);
        m_model->set("c", "pinned", true);

        m_source->removeEverySecond(); // drops "a" and "c" via layout change
        QCOMPARE(m_model->entries().size(), 1);
        QCOMPARE(m_model->get("a", "pinned"), QVariant(false));
        QCOMPARE(m_model->get("b", "pinned"), QVariant(true));

        m_source->reset({ { "key", { "b", "x" }}, { "name", { "B", "X" }} });
        QCOMPARE(m_model->entries().size(), 1);
        QCOMPARE(value(*m_model, 0, "pinned"), QVariant(true));
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(false));

        m_source->resetAndClear();
        QVERIFY(m_model->entries().isEmpty());
    }

    void sourceKeyChangeIsFollowed()
    {
        m_model->set("b", "pinned", true);
        const auto keyRole = roleId(*m_source, "key");
        const auto overlayRoles = QVector<int>{ roleId(*m_model, "pinned"),
                                                roleId(*m_model, "timestamp") };
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);

        // row 0 takes over key "b": its overlay values change, consumers notified
        m_source->update(0, keyRole, "b");
        QCOMPARE(value(*m_model, 0, "pinned"), QVariant(true));

        const auto overlaySignal = std::find_if(spy.cbegin(), spy.cend(), [&](auto& args) {
            auto roles = args.at(2).template value<QVector<int>>();
            std::sort(roles.begin(), roles.end());
            auto expected = overlayRoles;
            std::sort(expected.begin(), expected.end());
            return args.at(0).toModelIndex().row() == 0
                    && args.at(1).toModelIndex().row() == 0 && roles == expected;
        });
        QVERIFY(overlaySignal != spy.cend());

        // key changed in place is not pruned immediately (no scan per update)
        m_source->update(0, keyRole, "a");
        m_source->update(1, keyRole, "x");
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(false));
        QCOMPARE(m_model->entries().size(), 1);

        // ... but on the next structural change
        m_source->invert();
        QVERIFY(m_model->entries().isEmpty());
    }

    void noOverlaySignalOnKeyChangeWithoutValues()
    {
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);
        m_source->update(0, roleId(*m_source, "key"), "z");

        // only the forwarded source change
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(2).value<QVector<int>>(),
                 QVector<int>{ roleId(*m_source, "key") });
    }

    void structuralChangesAreSignalled()
    {
        m_model->set("b", "pinned", true);

        QSignalSpy layoutSpy(m_model.get(), &QAbstractItemModel::layoutChanged);
        m_source->invert();
        QCOMPARE(layoutSpy.count(), 1);
        QCOMPARE(value(*m_model, 1, "key"), QVariant("b"));
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(true));

        QSignalSpy resetSpy(m_model.get(), &QAbstractItemModel::modelReset);
        m_source->reset({ { "key", { "x", "b" }}, { "name", { "X", "B" }} });
        QCOMPARE(resetSpy.count(), 1);
        QCOMPARE(value(*m_model, 0, "pinned"), QVariant(false));
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(true));

        QSignalSpy removeSpy(m_model.get(), &QAbstractItemModel::rowsRemoved);
        m_source->remove(1);
        QCOMPARE(removeSpy.count(), 1);
        QVERIFY(m_model->entries().isEmpty());
    }

    // Updates of the source must not scan all keys, also when values are set
    // and the update re-sends the key role (as SFPM proxy roles do).
    void sourceUpdatesDontScanKeys()
    {
        constexpr int count = 1000;
        QVariantList keys, names;
        for (int i = 0; i < count; i++) {
            keys << QString::number(i);
            names << QString("name %1").arg(i);
        }

        TestModel source({ { "key", keys }, { "name", names } });
        RoleReadsCounter counter;
        counter.setSourceModel(&source);
        counter.role = roleId(source, "key");

        RolesOverlayModel model;
        model.setKeyRole("key");
        model.setDefaults({ { "pinned", false } });
        model.setSourceModel(&counter);

        model.set("10", "pinned", true);

        counter.reads = 0;
        for (int i = 0; i < count; i++)
            source.update(i, counter.role, QString::number(i));

        QCOMPARE(counter.reads, 0);

        // single key change: stops at the first match
        counter.reads = 0;
        model.set("10", "pinned", false);
        QCOMPARE(counter.reads, 11);
    }

    void resettingToDefaultDropsEntry()
    {
        m_model->set("a", "pinned", true);
        m_model->set("a", "timestamp", 5);
        QCOMPARE(m_model->entries().size(), 1);

        m_model->set("a", "pinned", false);
        QCOMPARE(m_model->entries().first().toMap(),
                 (QVariantMap{ { "key", "a" }, { "timestamp", 5 } }));

        m_model->set("a", "timestamp", 0);
        QVERIFY(m_model->entries().isEmpty());
    }

    void setEntriesNormalizesAndNotifies()
    {
        QSignalSpy spy(m_model.get(), &QAbstractItemModel::dataChanged);

        m_model->setEntries({
            QVariantMap{ { "key", "a" }, { "pinned", false }, { "timestamp", 0 } },
            QVariantMap{ { "key", "b" }, { "pinned", true }, { "timestamp", 7.0 } },
            QVariantMap{ { "key", "c" }, { "timestamp", 3 } },
            QVariantMap{ { "key", "absent" }, { "pinned", true } },
        });

        // single range covering rows 1 and 2
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toModelIndex().row(), 1);
        QCOMPARE(spy.at(0).at(1).toModelIndex().row(), 2);

        QCOMPARE(m_model->entries().size(), 2);
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(true));
        QCOMPARE(value(*m_model, 1, "timestamp"), QVariant(7.0));
        QCOMPARE(value(*m_model, 2, "timestamp"), QVariant(3));

        spy.clear();
        m_model->clear();
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_model->entries().isEmpty());
        QCOMPARE(value(*m_model, 1, "pinned"), QVariant(false));
    }

    void overridesExistingSourceRole()
    {
        RolesOverlayModel model;
        model.setKeyRole("key");
        model.setDefaults({ { "name", "default" } });
        model.setSourceModel(m_source.get());

        QCOMPARE(model.roleNames().size(), 2);
        QCOMPARE(value(model, 0, "name"), QVariant("default"));

        model.set("a", "name", "custom");
        QCOMPARE(value(model, 0, "name"), QVariant("custom"));
    }

    void rolesInitializedOnFirstInsertion()
    {
        TestModel source;
        RolesOverlayModel model;
        model.setKeyRole("key");
        model.setDefaults({ { "pinned", false } });
        model.setSourceModel(&source);

        QVERIFY(model.roleNames().isEmpty());

        source.appendAndInitRoles({ { "key", { "a" }} });

        QCOMPARE(model.roleNames().size(), 2);
        model.set("a", "pinned", true);
        QCOMPARE(value(model, 0, "pinned"), QVariant(true));
    }
};

QTEST_MAIN(TestRolesOverlayModel)
#include "tst_RolesOverlayModel.moc"

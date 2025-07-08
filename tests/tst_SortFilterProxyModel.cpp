#include <QTest>

#include <qtmodelstoolkit/testing/testmodel.h>
#include <qtmodelstoolkit/testing/modelsignalsspy.h>
#include <qtmodelstoolkit/testing/modeltestutils.h>

#include <qqmlsortfilterproxymodel.h>
#include <proxyroles/joinrole.h>

using namespace qtmt;
using SFPM = qqsfpm::QQmlSortFilterProxyModel;

class TestSortFilterProxyModel : public QObject
{
    Q_OBJECT

private slots:
    void basicInitTest()
    {
        SFPM sfpm;

        TestModel sourceModel({
            { "name", { "name 1_1", "name 1_2", "name 1_3", "name 1_4" }}
        });

        ModelSignalsSpy signalsSpy(&sfpm);
        QCOMPARE(signalsSpy.count(), 0);

        {
            QObject context;
            connect(&sfpm, &SFPM::modelAboutToBeReset, &context,
                    [&sfpm] {
                QCOMPARE(sfpm.roleNames().size(), 0);
                QCOMPARE(sfpm.rowCount(), 0);
            });

            sfpm.setSourceModel(&sourceModel);
        }

        QCOMPARE(signalsSpy.count(), 2);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 1);
        QVERIFY(isSame(sourceModel, sfpm));
    }

    void initWithEmptyModelWithNoRolesTest()
    {
        SFPM sfpm;

        TestModel sourceModel;

        ModelSignalsSpy signalsSpy(&sfpm);
        QCOMPARE(signalsSpy.count(), 0);

        sfpm.setSourceModel(&sourceModel);

        QCOMPARE(signalsSpy.count(), 2);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 1);
        QVERIFY(isSame(sourceModel, sfpm));

        {
            QObject context;
            connect(&sfpm, &SFPM::modelAboutToBeReset, &context,
                    [&sfpm] {
                        QCOMPARE(sfpm.roleNames().size(), 0);
                        QCOMPARE(sfpm.rowCount(), 0);
                    });

            sourceModel.appendAndInitRoles({
                { "name", { "name 1_1", "name 1_2", "name 1_3", "name 1_4" }}
            });
        }

        QCOMPARE(signalsSpy.count(), 4);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 2);
        QVERIFY(isSame(sourceModel, sfpm));

        {
            QObject context;
            connect(&sfpm, &SFPM::rowsAboutToBeInserted, &context,
                    [&sfpm] {
                        QCOMPARE(sfpm.roleNames().size(), 1);
                        QCOMPARE(sfpm.rowCount(), 4);
                    });

            sourceModel.append({ "name 1_5" });
        }

        QCOMPARE(signalsSpy.count(), 6);
        QCOMPARE(signalsSpy.rowsAboutToBeInsertedSpy.count(), 1);
        QCOMPARE(signalsSpy.rowsInsertedSpy.count(), 1);
        QCOMPARE(sfpm.rowCount(), 5);
    }

    void initWithEmptyModelAndClearSourceTest()
    {
        SFPM sfpm;

        TestModel sourceModel;
        ModelSignalsSpy signalsSpy(&sfpm);
        QCOMPARE(signalsSpy.count(), 0);

        sfpm.setSourceModel(&sourceModel);

        QCOMPARE(signalsSpy.count(), 2);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 1);
        QVERIFY(isSame(sourceModel, sfpm));

        sfpm.setSourceModel(nullptr);
        QCOMPARE(signalsSpy.count(), 4);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 2);

        // make sure that actions on the source don't affect the sfpm (that
        // signals are properly disconnected internally)
        sourceModel.appendAndInitRoles({
            { "name", { "name 1_1", "name 1_2", "name 1_3", "name 1_4" }}
        });

        QCOMPARE(signalsSpy.count(), 4);
    }

    void initWithEmptyModelAndResetSourceTest()
    {
        SFPM sfpm;

        TestModel sourceModel;
        ModelSignalsSpy signalsSpy(&sfpm);
        QCOMPARE(signalsSpy.count(), 0);

        sfpm.setSourceModel(&sourceModel);

        QCOMPARE(signalsSpy.count(), 2);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 1);
        QVERIFY(isSame(sourceModel, sfpm));

        // reset source to have roles defined, no content
        {
            QObject context;
            connect(&sfpm, &SFPM::modelAboutToBeReset, &context,
                    [&sfpm] {
                        QCOMPARE(sfpm.roleNames().size(), 0);
                        QCOMPARE(sfpm.rowCount(), 0);
                    });

            sourceModel.reset({
                { "name", {}}
            });
        }

        QCOMPARE(signalsSpy.count(), 4);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 2);

        // insertion should be handled in standard way, no model reset
        {
            QObject context;
            connect(&sfpm, &SFPM::rowsAboutToBeInserted, &context,
                    [&sfpm] {
                        QCOMPARE(sfpm.roleNames().size(), 1);
                        QCOMPARE(sfpm.rowCount(), 0);
                    });

            sourceModel.append({ "name_1" });
        }

        QCOMPARE(signalsSpy.count(), 6);
        QCOMPARE(signalsSpy.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy.modelResetSpy.count(), 2);
        QCOMPARE(signalsSpy.rowsAboutToBeInsertedSpy.count(), 1);
        QCOMPARE(signalsSpy.rowsInsertedSpy.count(), 1);
        QCOMPARE(sfpm.count(), 1);
    }

    void changingSourceOnChainedSfpmTest()
    {
        SFPM sfpm_1;
        SFPM sfpm_2;

        ModelSignalsSpy signalsSpy_1(&sfpm_1);
        ModelSignalsSpy signalsSpy_2(&sfpm_2);

        sfpm_2.setSourceModel(&sfpm_1);

        QCOMPARE(signalsSpy_1.count(), 0);
        QCOMPARE(signalsSpy_2.count(), 2);
        QCOMPARE(signalsSpy_2.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy_2.modelResetSpy.count(), 1);

        TestModel sourceModel_1;
        TestModel sourceModel_2;

        sfpm_1.setSourceModel(&sourceModel_1);

        QCOMPARE(signalsSpy_1.count(), 2);
        QCOMPARE(signalsSpy_1.modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(signalsSpy_1.modelResetSpy.count(), 1);
        QCOMPARE(signalsSpy_2.count(), 4);
        QCOMPARE(signalsSpy_2.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy_2.modelResetSpy.count(), 2);

        sourceModel_1.appendAndInitRoles({
            { "name", { "name 1_1", "name 1_2", "name 1_3", "name 1_4" }}
        });

        QCOMPARE(signalsSpy_1.count(), 4);
        QCOMPARE(signalsSpy_1.modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(signalsSpy_1.modelResetSpy.count(), 2);
        QCOMPARE(signalsSpy_2.count(), 6);
        QCOMPARE(signalsSpy_2.modelAboutToBeResetSpy.count(), 3);
        QCOMPARE(signalsSpy_2.modelResetSpy.count(), 3);
        QCOMPARE(sfpm_1.roleNames().count(), 1);
        QCOMPARE(sfpm_2.roleNames().count(), 1);

        sfpm_1.setSourceModel(&sourceModel_2);

        QCOMPARE(signalsSpy_1.count(), 6);
        QCOMPARE(signalsSpy_1.modelAboutToBeResetSpy.count(), 3);
        QCOMPARE(signalsSpy_1.modelResetSpy.count(), 3);
        QCOMPARE(signalsSpy_2.count(), 8);
        QCOMPARE(signalsSpy_2.modelAboutToBeResetSpy.count(), 4);
        QCOMPARE(signalsSpy_2.modelResetSpy.count(), 4);
        QCOMPARE(sfpm_1.roleNames().count(), 0);
        QCOMPARE(sfpm_2.roleNames().count(), 0);

        sourceModel_2.appendAndInitRoles({
            { "name", { "name 1_1", "name 1_2" }},
            { "subname", { "subname 1_1", "subname 1_2" }}
        });

        QCOMPARE(signalsSpy_1.count(), 8);
        QCOMPARE(signalsSpy_1.modelAboutToBeResetSpy.count(), 4);
        QCOMPARE(signalsSpy_1.modelResetSpy.count(), 4);
        QCOMPARE(signalsSpy_2.count(), 10);
        QCOMPARE(signalsSpy_2.modelAboutToBeResetSpy.count(), 5);
        QCOMPARE(signalsSpy_2.modelResetSpy.count(), 5);
        QCOMPARE(sfpm_1.roleNames().count(), 2);
        QCOMPARE(sfpm_2.roleNames().count(), 2);
    }

    void connectionsCleanupTest()
    {
        // This test would crash in case of lack of internal connections
        // cleanup, triggering actions on the destroyed object.
        TestModel sourceModel;

        {
            SFPM sfpm;
            sfpm.setSourceModel(&sourceModel);
        }

        sourceModel.appendAndInitRoles({
            { "name", { "name 1_1", "name 1_2", "name 1_3", "name 1_4" }}
        });
    }
};

QTEST_MAIN(TestSortFilterProxyModel)
#include "tst_SortFilterProxyModel.moc"

#include <QSignalSpy>
#include <QTest>

#include <memory>

#include <qtmodelstoolkit/rolesrenamingmodel.h>
#include <qtmodelstoolkit/testing/testmodel.h>

using namespace qtmt;

class TestRolesRenamingModel: public QObject
{
    Q_OBJECT

private slots:
    void initializationWithBrokenMappingTest()
    {
        TestModel sourceModel({"id", "name", "color"});
        RolesRenamingModel model;

        QQmlListProperty<RoleRename> mapping = model.mapping();

        RoleRename rename;
        rename.setFrom("someIdFrom");
        rename.setTo("someIdTo");

        mapping.append(&mapping, &rename);

        model.setSourceModel(&sourceModel);

        QTest::ignoreMessage(QtWarningMsg,
                             "RolesRenamingModel: specified source roles not "
                             "found: \"someIdFrom\"!");

        QHash<int, QByteArray> expectedRoles = {
            {0, "id"}, {1, "name"}, {2, "color"}
        };
        QCOMPARE(model.roleNames(), expectedRoles);
    }

    void remappingTest()
    {
        TestModel sourceModel({"id", "name", "color"});
        RolesRenamingModel model;

        QQmlListProperty<RoleRename> mapping = model.mapping();

        RoleRename rename_1;
        rename_1.setFrom("id");
        rename_1.setTo("tokenId");
        mapping.append(&mapping, &rename_1);

        RoleRename rename_2;
        rename_2.setFrom("name");
        rename_2.setTo("tokenName");
        mapping.append(&mapping, &rename_2);

        model.setSourceModel(&sourceModel);

        QHash<int, QByteArray> expectedRoles = {
            {0, "tokenId"}, {1, "tokenName"}, {2, "color"}
        };
        QCOMPARE(model.roleNames(), expectedRoles);
    }

    void addMappingAfterFetchingRoleNamesTest()
    {
        TestModel sourceModel({"id", "name", "color"});
        RolesRenamingModel model;

        QQmlListProperty<RoleRename> mapping = model.mapping();

        RoleRename rename_1;
        rename_1.setFrom("id");
        rename_1.setTo("tokenId");
        mapping.append(&mapping, &rename_1);

        model.setSourceModel(&sourceModel);

        QHash<int, QByteArray> expectedRoles = {
            {0, "tokenId"}, {1, "name"}, {2, "color"}
        };
        QCOMPARE(model.roleNames(), expectedRoles);

        RoleRename rename_2;
        rename_2.setFrom("name");
        rename_2.setTo("tokenName");

        QTest::ignoreMessage(QtWarningMsg,
                             "RolesRenamingModel: role names mapping cannot be "
                             "modified after fetching role names!");
        mapping.append(&mapping, &rename_2);

        QCOMPARE(model.roleNames(), expectedRoles);
    }

    void duplicatedNamesTest()
    {
        TestModel sourceModel({"id", "name", "color"});
        RolesRenamingModel model;

        QQmlListProperty<RoleRename> mapping = model.mapping();

        RoleRename rename_1;
        rename_1.setFrom("id");
        rename_1.setTo("name");
        mapping.append(&mapping, &rename_1);

        model.setSourceModel(&sourceModel);

        QTest::ignoreMessage(QtWarningMsg,
                             "RolesRenamingModel: model cannot contain "
                             "duplicated role names!");

        QCOMPARE(model.roleNames(), {});
    }

    void resettingFromToPropertiesTest()
    {
        RoleRename rename;

        rename.setFrom("id");
        QCOMPARE(rename.from(), "id");
        QCOMPARE(rename.to(), "");

        QTest::ignoreMessage(QtWarningMsg,
                             "RoleRename: property \"from\" is intended to be "
                             "initialized once and not changed!");
        rename.setFrom("id2");
        QCOMPARE(rename.from(), "id");
        QCOMPARE(rename.to(), "");

        rename.setTo("myId");
        QCOMPARE(rename.from(), "id");
        QCOMPARE(rename.to(), "myId");

        QTest::ignoreMessage(QtWarningMsg,
                             "RoleRename: property \"to\" is intended to be "
                             "initialized once and not changed!");
        rename.setTo("myId2");
        QCOMPARE(rename.from(), "id");
        QCOMPARE(rename.to(), "myId");
    }

    void sourceModelDeletedTest()
    {
        auto sourceModel = std::make_unique<TestModel>(
            QList<QPair<QString, QVariantList>> {
               { "id", { 1, 2, 3 }},
               { "name", { "a", "b", "c" }},
               { "color", { "red", "green", "blue" }}
            });

        RolesRenamingModel model;

        QQmlListProperty<RoleRename> mapping = model.mapping();

        RoleRename rename_1;
        rename_1.setFrom("id");
        rename_1.setTo("tokenId");
        mapping.append(&mapping, &rename_1);

        RoleRename rename_2;
        rename_2.setFrom("name");
        rename_2.setTo("tokenName");
        mapping.append(&mapping, &rename_2);

        model.setSourceModel(sourceModel.get());

        QHash<int, QByteArray> expectedRoles = {
            {0, "tokenId"}, {1, "tokenName"}, {2, "color"}
        };
        QCOMPARE(model.roleNames(), expectedRoles);
        QCOMPARE(model.rowCount(), 3);

        QCOMPARE(model.data(model.index(0, 0), 0), 1);
        QCOMPARE(model.data(model.index(0, 0), 1), "a");
        QCOMPARE(model.data(model.index(2, 0), 0), 3);
        QCOMPARE(model.data(model.index(2, 0), 1), "c");

        QSignalSpy destroyedSpy(sourceModel.get(), &QObject::destroyed);
        sourceModel.reset();

        QCOMPARE(destroyedSpy.size(), 1);

        QCOMPARE(model.roleNames(), {});
        QCOMPARE(model.rowCount(), 0);

        QCOMPARE(model.roleNames(), {});
        QCOMPARE(model.data(model.index(0, 0), 0), {});
        QCOMPARE(model.data(model.index(0, 0), 1), {});
        QCOMPARE(model.data(model.index(2, 0), 0), {});
        QCOMPARE(model.data(model.index(2, 0), 1), {});
    }
};

QTEST_MAIN(TestRolesRenamingModel)
#include "tst_RolesRenamingModel.moc"

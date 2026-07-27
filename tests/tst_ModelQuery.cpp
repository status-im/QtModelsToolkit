#include <QTest>

#include <QCoreApplication>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QRegularExpression>

#include <qtmodelstoolkit/modelquery.h>
#include <qtmodelstoolkit/testing/listmodelwrapper.h>

using namespace qtmt;

class TestModelQuery : public QObject
{
    Q_OBJECT

private slots:
    void testIndexOf()
    {
        QQmlEngine engine;

        ListModelWrapper model(engine, R"([
            { name: "A", balance: 1, valid: true },
            { name: "B", balance: 2, valid: false },
            { name: "B", balance: 3, valid: false },
            { name: "C", balance: 4, valid: false }
        ])");

        ModelQuery utils;

        QCOMPARE(utils.indexOf(nullptr, "name", "A"), -1);
        QCOMPARE(utils.indexOf(model, "notExisting", "A"), -1);

        QCOMPARE(utils.indexOf(model, "name", "A"), 0);
        QCOMPARE(utils.indexOf(model, "name", "B"), 1);
        QCOMPARE(utils.indexOf(model, "name", "C"), 3);
        QCOMPARE(utils.indexOf(model, "name", "D"), -1);

        QCOMPARE(utils.indexOf(model, "valid", true), 0);
        QCOMPARE(utils.indexOf(model, "valid", false), 1);
        QCOMPARE(utils.indexOf(model, "valid", "true"), 0);
        QCOMPARE(utils.indexOf(model, "valid", "false"), 1);
        QCOMPARE(utils.indexOf(model, "valid", 1), 0);
        QCOMPARE(utils.indexOf(model, "valid", 0), 1);

        QCOMPARE(utils.indexOf(model, "balance", 1), 0);
        QCOMPARE(utils.indexOf(model, "balance", 2), 1);
        QCOMPARE(utils.indexOf(model, "balance", 3), 2);
        QCOMPARE(utils.indexOf(model, "balance", 4), 3);
        QCOMPARE(utils.indexOf(model, "balance", "4"), 3);
    }

    void testGetDoesNotHandSubmodelToJsGc()
    {
        QQmlEngine engine;

        ListModelWrapper model(engine, R"([
            { name: "A", balances: [ { chainId: 1 }, { chainId: 2 } ] }
        ])");

        ModelQuery utils;

        // The submodel wrapper is cached, so the direct call returns the same
        // object the JS call below returns.
        QPointer<QObject> subModel = utils.get(model, 0, "balances").value<QObject *>();
        QVERIFY(subModel);

        // Fetch through the JS engine: a QVariant(QObject*) returned from a
        // Q_INVOKABLE is handed to the JS GC unless ownership is pinned.
        engine.rootContext()->setContextProperty("modelQuery", &utils);
        engine.rootContext()->setContextProperty("sourceModel", model.model());
        QQmlExpression expression(engine.rootContext(), nullptr,
                                  QStringLiteral("modelQuery.get(sourceModel, 0, 'balances')"));
        QVERIFY(!expression.evaluate().isNull());

        QCOMPARE(QQmlEngine::objectOwnership(subModel), QQmlEngine::CppOwnership);

        // The source model owns the submodel; a GC pass must not delete it.
        engine.collectGarbage();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(subModel);
    }

    void testGetRespectsExplicitJsOwnershipAndWarns()
    {
        QQmlEngine engine;

        ListModelWrapper model(engine, R"([
            { name: "A", balances: [ { chainId: 1 } ] }
        ])");

        ModelQuery utils;

        QObject *subModel = utils.get(model, 0, "balances").value<QObject *>();
        QVERIFY(subModel);
        QQmlEngine::setObjectOwnership(subModel, QQmlEngine::JavaScriptOwnership);

        engine.rootContext()->setContextProperty("modelQuery", &utils);
        engine.rootContext()->setContextProperty("sourceModel", model.model());
        QQmlExpression expression(engine.rootContext(), nullptr,
                                  QStringLiteral("modelQuery.get(sourceModel, 0, 'balances')"));

        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(QStringLiteral("ModelQuery::get:.*JS-owned")));
        QVERIFY(!expression.evaluate().isNull());

        QCOMPARE(QQmlEngine::objectOwnership(subModel), QQmlEngine::JavaScriptOwnership);
    }
};

QTEST_MAIN(TestModelQuery)
#include "tst_ModelQuery.moc"

#include <QQmlEngine>
#include <QCoreApplication>

#include "qtmodelstoolbox/concatmodel.h"

static void registerQmlTypes() {
    qDebug() << "XXXX";
    qmlRegisterType<ConcatModel>("QtModelsToolkit", 1, 0, "ConcatModel");
}

Q_COREAPP_STARTUP_FUNCTION(registerQmlTypes)

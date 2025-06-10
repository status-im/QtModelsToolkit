#include <QQmlEngine>
#include <QCoreApplication>

#include "qtmodelstoolkit/concatmodel.h"
#include "qtmodelstoolkit/functionaggregator.h"
#include "qtmodelstoolkit/groupingmodel.h"
#include "qtmodelstoolkit/leftjoinmodel.h"
#include "qtmodelstoolkit/modelcount.h"
#include "qtmodelstoolkit/modelentry.h"
#include "qtmodelstoolkit/modelquery.h"
#include "qtmodelstoolkit/movablemodel.h"
#include "qtmodelstoolkit/objectproxymodel.h"
#include "qtmodelstoolkit/rolesrenamingmodel.h"
#include "qtmodelstoolkit/snapshotobject.h"
#include "qtmodelstoolkit/sumaggregator.h"
#include "qtmodelstoolkit/writableproxymodel.h"

namespace qtmt {

void registerQmlTypes() {
    qmlRegisterSingletonType<ModelQuery>("QtModelsToolkit", 1, 0,
                                         "ModelQuery", [](QQmlEngine*, QJSEngine*) {
        return new ModelQuery;
    });
    qmlRegisterType<ConcatModel>("QtModelsToolkit", 1, 0, "ConcatModel");
    qmlRegisterType<FunctionAggregator>("QtModelsToolkit", 1, 0, "FunctionAggregator");
    qmlRegisterType<GroupingModel>("QtModelsToolkit", 1, 0, "GroupingModel");
    qmlRegisterType<LeftJoinModel>("QtModelsToolkit", 1, 0, "LeftJoinModel");
    qmlRegisterType<ModelEntry>("QtModelsToolkit", 1, 0, "ModelEntry");
    qmlRegisterType<MovableModel>("QtModelsToolkit", 1, 0, "MovableModel");
    qmlRegisterType<ObjectProxyModel>("QtModelsToolkit", 1, 0, "ObjectProxyModel");
    qmlRegisterType<RoleRename>("QtModelsToolkit", 1, 0, "RoleRename");
    qmlRegisterType<RolesRenamingModel>("QtModelsToolkit", 1, 0, "RolesRenamingModel");
    qmlRegisterType<SnapshotObject>("QtModelsToolkit", 1, 0, "SnapshotObject");
    qmlRegisterType<SourceModel>("QtModelsToolkit", 1, 0, "SourceModel");
    qmlRegisterType<SumAggregator>("QtModelsToolkit", 1, 0, "SumAggregator");
    qmlRegisterType<WritableProxyModel>("QtModelsToolkit", 1, 0, "WritableProxyModel");
    qmlRegisterUncreatableType<ModelCount>("QtModelsToolkit", 1, 0,
                                           "ModelCount", QStringLiteral("This is attached type, cannot be created directly."));
}

Q_COREAPP_STARTUP_FUNCTION(registerQmlTypes)

} // namespace qtmt

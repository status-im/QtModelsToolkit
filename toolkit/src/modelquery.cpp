#include "qtmodelstoolkit/modelquery.h"

#include <QAbstractItemModel>
#include <QDebug>

namespace qtmt {

/*!
    \qmltype ModelQuery
    \inherits QObject
    \inqmlmodule QtModelsToolkit
    \ingroup Utilities
    \brief Set of methods for querying QAbstractItemModel-based models.
*/
ModelQuery::ModelQuery(QObject* parent)
    : QObject(parent)
{
}

/*!
    \qmlmethod bool ModelQuery::isModel(var obj)

    Return true if obj is a model based on \c QAbstractItemModel.
*/
bool ModelQuery::isModel(const QVariant &obj) const
{
    if (!obj.canConvert<QObject*>())
        return false;

    return qobject_cast<QAbstractItemModel*>(obj.value<QObject*>()) != nullptr;
}

/*!
    \qmlmethod list<string> ModelQuery::roleNames(var model)

    Return role names a string list for given \a model.
*/
QStringList ModelQuery::roleNames(QAbstractItemModel *model) const
{
    if (model == nullptr)
        return {};

    const auto roles = model->roleNames();
    return {roles.cbegin(), roles.cend()};
}

/*!
    \qmlmethod int ModelQuery::roleByName(var model, string roleName)

    Returns value of the role for given \a model and \a roleName.
*/
int ModelQuery::roleByName(QAbstractItemModel* model,
                           const QString &roleName) const
{
    if (model == nullptr)
        return -1;

    return model->roleNames().key(roleName.toUtf8(), -1);
}


/*!
    \qmlmethod object ModelQuery::get(var model, string roleName)

    Returns values (map role name -> value) for given \a row from a \a model.
*/
QVariantMap ModelQuery::get(QAbstractItemModel *model, int row) const
{
    QVariantMap map;

    if (model == nullptr)
        return map;

    const auto modelIndex = model->index(row, 0);
    const auto roles = model->roleNames();

    for (auto it = roles.begin(); it != roles.end(); ++it)
        map.insert(it.value(), model->data(modelIndex, it.key()));

    return map;
}

/*!
    \qmlmethod object ModelQuery::get(var model, string roleName)

    Returns value for given \a row and \a roleName from a \a model.
*/
QVariant ModelQuery::get(QAbstractItemModel *model,
                                 int row, const QString &roleName) const
{
    if (auto role = roleByName(model, roleName); role != -1)
        return model->data(model->index(row, 0), role);

    return {};
}

/*!
    \qmlmethod int ModelQuery::indexOf(var model, string roleName, var value)

    Returns the index of given value of given value in the model using
    QAbstractItemModel::match method with \c Qt::MatchExactly flag.

    \note \c QAbstractItemModel::match with \c Qt::MatchExactly flag performs
    \c{QVariant}-based matching internally. It means that types are not compared
    and e.g. 4 (int) compared to string "4" will give a positive result.
*/
int ModelQuery::indexOf(QAbstractItemModel* model,
                        const QString& roleName, const QVariant& value) const
{
    auto role = roleByName(model, roleName);

    if (role == -1 || model->rowCount() == 0)
        return -1;

    QModelIndexList indexes = model->match(model->index(0, 0), role, value, 1,
                                           Qt::MatchExactly);

    if (indexes.isEmpty())
        return -1;

    return indexes.first().row();
}

/*!
    \qmlmethod int ModelQuery::indexOf(var model, int row)

    Returns \c QPersistentModelIndex for a given \a row.
*/
QPersistentModelIndex ModelQuery::persistentIndex(QAbstractItemModel* model,
                                                  int row)
{
    if (!model)
        return {};
    return {model->index(row, 0)};
}

/*!
    \qmlmethod int ModelQuery::contains(var model, string roleName, var value, int mode)

    Checks if \a model contains given \a value for a \a roleName.

    \a mode can be \c Qt.CaseSensitive or \c Qt.CaseInsensitive.
*/
bool ModelQuery::contains(QAbstractItemModel* model, const QString& roleName,
                          const QVariant& value, int mode) const
{
    if(!model) return false;

    Qt::MatchFlags flags = Qt::MatchFixedString; // Qt::CaseInsensitive by default
    if(mode == Qt::CaseSensitive) flags |= Qt::MatchCaseSensitive;

    auto role = roleByName(model, roleName);

    if (role == -1)
        return false;

    const auto indexes = model->match(model->index(0, 0), role, value, 1, flags);
    return !indexes.isEmpty();
}

} // namespace qtmt

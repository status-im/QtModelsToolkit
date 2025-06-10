#include "qtmodelstoolkit/rolesrenamingmodel.h"

#include <QDebug>

namespace qtmt {

/*!
    \qmltype RolesRenamingModel
    \inherits QIdentityProxyModel
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModels
    \brief Simple proxy allowing to rename roles according to a provided mapping.

    In the following example original roles \c key and \c title are renamed
    to \c id and \c name, respectively.

    \qml
    RolesRenamingModel {
        id: renamedModel

        sourceModel: sourceModel

        mapping: [
            RoleRename {
                from: "key"
                to: "id"
            },
            RoleRename {
                from: "title"
                to: "name"
            }
        ]
    }
    \endqml
*/

/*!
    \qmltype RoleRename
    \inherits QtObject
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModelsAssociated
    \brief Defines name mapping for a single role within \l {RolesRenamingModel}.

    This helper component allows specify name of existing role (old name) and
    the new name which will be exposed from \l {RolesRenamingModel}.
*/

RoleRename::RoleRename(QObject* parent)
    : QObject{parent}
{
}

void RoleRename::setFrom(const QString& from)
{
    if (m_from == from)
        return;

    if (!m_from.isEmpty()) {
        qWarning() << "RoleRename: property \"from\" is intended to be "
                      "initialized once and not changed!";
        return;
    }

    m_from = from;
    emit fromChanged();
}

/*!
    \qmlproperty string RoleRename::from

    This property specifies the role name exising in the source model that is
    intended to be renamed.
*/
const QString& RoleRename::from() const
{
    return m_from;
}

void RoleRename::setTo(const QString& to)
{
    if (m_to == to)
        return;

    if (!m_to.isEmpty()) {
        qWarning() << "RoleRename: property \"to\" is intended to be "
                      "initialized once and not changed!";
        return;
    }

    m_to = to;
    emit toChanged();
}

/*!
    \qmlproperty string RoleRename::to

    This property specifies the new name for the role.
*/
const QString& RoleRename::to() const
{
    return m_to;
}

RolesRenamingModel::RolesRenamingModel(QObject* parent)
    : QIdentityProxyModel{parent}
{
}

/*!
    \qmlproperty list<RoleName> RolesRenamingModel::mapping

    This property holds the mapping as a list of \l RoleRename objects.
*/
QQmlListProperty<RoleRename> RolesRenamingModel::mapping()
{
    QQmlListProperty<RoleRename> list(this, &m_mapping);

    list.replace = nullptr;
    list.clear = nullptr;
    list.removeLast = nullptr;

    list.append = [](auto listProperty, auto element) {
        RolesRenamingModel* model = qobject_cast<RolesRenamingModel*>(
                    listProperty->object);

        if (model->m_rolesFetched) {
            qWarning() << "RolesRenamingModel: role names mapping cannot be "
                          "modified after fetching role names!";
            return;
        }

        model->m_mapping.append(element);
    };

    return list;
}

QHash<int, QByteArray> RolesRenamingModel::roleNames() const
{
    const auto roles = sourceModel() ? sourceModel()->roleNames() : QHash<int, QByteArray>{};

    if (roles.isEmpty())
        return roles;

    QHash<QString, RoleRename*> renameMap;

    for (const auto rename : m_mapping)
        renameMap.insert(rename->from(), rename);

    QHash<int, QByteArray> remapped;
    remapped.reserve(roles.size());

    QSet<QByteArray> roleNamesSet;
    roleNamesSet.reserve(roles.size());

    for (auto i = roles.cbegin(), end = roles.cend(); i != end; ++i) {
        RoleRename* rename = renameMap.take(i.value());
        QByteArray roleName = rename ? rename->to().toUtf8() : i.value();

        remapped.insert(i.key(), roleName);
        roleNamesSet.insert(roleName);
    }

    if (roles.size() != roleNamesSet.size()) {
        qWarning() << "RolesRenamingModel: model cannot contain duplicated "
                      "role names!";
        return {};
    }

    if (!renameMap.isEmpty()) {
        qWarning().nospace()
                << "RolesRenamingModel: specified source roles not found: "
                << renameMap.keys().join(" ") << "!";
    }

    m_rolesFetched = true;
    return remapped;
}

} // namespace qtmt

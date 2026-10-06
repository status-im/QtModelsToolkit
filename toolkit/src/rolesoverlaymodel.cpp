#include "qtmodelstoolkit/rolesoverlaymodel.h"

#include <QDebug>

#include <algorithm>

namespace qtmt {

/*!
    \qmltype RolesOverlayModel
    \inherits QIdentityProxyModel
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModels
    \brief Identity proxy adding writable roles addressed by a key role.

    Roles listed in \l defaults are added to the source roles (or override
    source roles of the same name). Values written via \l set are stored per
    key, not per row: no per-row objects or state are created, so sorting or
    filtering on those roles doesn't materialize anything. Rows without a stored
    value return the default.

    Values are kept only for keys present in the source model. They follow rows
    when moved, and are dropped when the key is gone from the source (row
    removed, reset, layout change or key change). Keys are expected to be
    unique.

    \qml
    RolesOverlayModel {
        sourceModel: entriesModel
        keyRole: "key"
        defaults: ({ pinned: false, timestamp: 0 })
    }
    \endqml

    Compared to \l WritableProxyModel, which tracks edits of rows (dirty
    state, inserted and removed rows), values here are bound to the key,
    so they don't depend on the identity of source rows across resets.
*/
RolesOverlayModel::RolesOverlayModel(QObject* parent)
    : QIdentityProxyModel{parent}
{
}

/*!
    \qmlproperty string RolesOverlayModel::keyRole

    Name of the source role identifying rows.
*/
const QString& RolesOverlayModel::keyRole() const
{
    return m_keyRole;
}

void RolesOverlayModel::setKeyRole(const QString& keyRole)
{
    if (m_keyRole == keyRole)
        return;

    beginResetModel();
    m_keyRole = keyRole;
    m_values.clear();
    endResetModel();

    emit keyRoleChanged();
}

/*!
    \qmlproperty object RolesOverlayModel::defaults

    Map of the overlay role names to their default values.
*/
const QVariantMap& RolesOverlayModel::defaults() const
{
    return m_defaults;
}

void RolesOverlayModel::setDefaults(const QVariantMap& defaults)
{
    if (m_defaults == defaults)
        return;

    beginResetModel();
    m_defaults = defaults;
    m_values.clear();
    endResetModel();

    emit defaultsChanged();
}

/*!
    \qmlmethod RolesOverlayModel::set(key, role, value)

    Sets the value of the overlay \a role for the row identified by \a key.
    Ignored when no such row is present.
*/
void RolesOverlayModel::set(const QString& key, const QString& role,
                            const QVariant& value)
{
    if (!m_defaults.contains(role)) {
        qWarning() << "RolesOverlayModel: role" << role << "has no default value";
        return;
    }

    const auto previous = m_values.value(key);
    const auto defaultValue = m_defaults.value(role);
    const auto current = previous.value(role, defaultValue);

    if (current == value)
        return;

    auto& entry = m_values[key];

    if (value == defaultValue)
        entry.remove(role);
    else
        entry.insert(role, value);

    if (entry.isEmpty())
        m_values.remove(key);

    if (!emitOverlayChanged({ key }, { m_overlayRoles.key(role, -1) })) {
        if (previous.isEmpty())
            m_values.remove(key);
        else
            m_values.insert(key, previous);
    }
}

/*!
    \qmlmethod RolesOverlayModel::get(key, role)

    Returns the value of the overlay \a role for \a key, or its default.
*/
QVariant RolesOverlayModel::get(const QString& key, const QString& role) const
{
    const auto it = m_values.constFind(key);

    if (it != m_values.cend() && it->contains(role))
        return it->value(role);

    return m_defaults.value(role);
}

/*!
    \qmlmethod RolesOverlayModel::entries()

    Returns non-default values as a list of objects with the "key" property and
    the overlay roles that differ from their defaults.
*/
QVariantList RolesOverlayModel::entries() const
{
    QVariantList result;
    result.reserve(m_values.size());

    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it) {
        QVariantMap entry = it.value();
        entry.insert(QStringLiteral("key"), it.key());
        result.append(entry);
    }

    return result;
}

/*!
    \qmlmethod RolesOverlayModel::setEntries(entries)

    Replaces all values with \a entries, a list of objects with the "key"
    property and values of overlay roles. Entries for keys not present in the
    source are ignored.
*/
void RolesOverlayModel::setEntries(const QVariantList& entries)
{
    const auto present = presentKeys();
    QHash<QString, QVariantMap> values;

    for (const auto& entryVariant : entries) {
        const auto entry = entryVariant.toMap();
        const auto key = entry.value(QStringLiteral("key")).toString();

        if (!present.contains(key))
            continue;

        for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it) {
            const auto valueIt = entry.constFind(it.key());

            if (valueIt != entry.cend() && valueIt.value() != it.value())
                values[key].insert(it.key(), valueIt.value());
        }
    }

    QSet<QString> affected;
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it)
        affected.insert(it.key());
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        affected.insert(it.key());

    m_values = std::move(values);
    emitOverlayChanged(affected, m_overlayRoles.keys().toVector());
}

/*!
    \qmlmethod RolesOverlayModel::clear()

    Resets all overlay roles to their defaults.
*/
void RolesOverlayModel::clear()
{
    setEntries({});
}

void RolesOverlayModel::setSourceModel(QAbstractItemModel* model)
{
    if (sourceModel() == model)
        return;

    if (sourceModel() != nullptr)
        disconnect(sourceModel(), nullptr, this, nullptr);

    m_values.clear();
    m_removedKeys.clear();

    if (model != nullptr) {
        connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this,
                [this](const QModelIndex& parent, int first, int last) {
            if (parent.isValid() || m_values.isEmpty())
                return;

            for (int row = first; row <= last; row++) {
                auto key = keyAt(row);

                if (m_values.contains(key))
                    m_removedKeys.insert(key);
            }
        });

        connect(model, &QAbstractItemModel::rowsRemoved, this, [this] {
            if (m_removedKeys.isEmpty())
                return;

            const auto present = presentKeys();

            for (const auto& key : std::as_const(m_removedKeys))
                if (!present.contains(key))
                    m_values.remove(key);

            m_removedKeys.clear();
        });

        connect(model, &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex&, const QModelIndex&,
                       const QVector<int>& roles) {
            if (roles.isEmpty() || roles.contains(m_keyRoleId))
                pruneToPresentKeys();
        });
        connect(model, &QAbstractItemModel::modelReset,
                this, &RolesOverlayModel::pruneToPresentKeys);
        connect(model, &QAbstractItemModel::layoutChanged,
                this, &RolesOverlayModel::pruneToPresentKeys);

        // Workaround for QTBUG-57971, roles may be known only after the first
        // insertion
        if (model->roleNames().isEmpty()) {
            connect(model, &QAbstractItemModel::rowsInserted, this, [this] {
                if (m_roleNames.isEmpty() && !sourceModel()->roleNames().isEmpty())
                    updateRoleNames();
            });
        }
    }

    QIdentityProxyModel::setSourceModel(model);
}

QHash<int, QByteArray> RolesOverlayModel::roleNames() const
{
    return m_roleNames;
}

QVariant RolesOverlayModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const auto overlayIt = m_overlayRoles.constFind(role);

    if (overlayIt == m_overlayRoles.cend())
        return QIdentityProxyModel::data(index, role);

    if (m_values.isEmpty())
        return m_defaults.value(overlayIt.value());

    return get(keyAt(index.row()), overlayIt.value());
}

void RolesOverlayModel::resetInternalData()
{
    QIdentityProxyModel::resetInternalData();
    updateRoleNames();
}

void RolesOverlayModel::updateRoleNames()
{
    m_roleNames.clear();
    m_overlayRoles.clear();
    m_keyRoleId = -1;

    if (sourceModel() == nullptr)
        return;

    auto roles = sourceModel()->roleNames();

    if (roles.isEmpty())
        return;

    m_keyRoleId = roles.key(m_keyRole.toUtf8(), -1);

    const auto ids = roles.keys();
    auto maxRole = *std::max_element(ids.cbegin(), ids.cend());

    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it) {
        const auto name = it.key().toUtf8();
        auto id = roles.key(name, -1);

        if (id == -1) {
            id = ++maxRole;
            roles.insert(id, name);
        }

        m_overlayRoles.insert(id, it.key());
    }

    m_roleNames = roles;
}

QString RolesOverlayModel::keyAt(int row) const
{
    // Not cached per row on purpose: no per-row state that could go out of
    // sync with the source.
    return sourceModel()->data(sourceModel()->index(row, 0), m_keyRoleId).toString();
}

QSet<QString> RolesOverlayModel::presentKeys() const
{
    QSet<QString> keys;

    if (sourceModel() == nullptr || m_keyRoleId == -1)
        return keys;

    const auto count = sourceModel()->rowCount();
    keys.reserve(count);

    for (int row = 0; row < count; row++)
        keys.insert(keyAt(row));

    return keys;
}

void RolesOverlayModel::pruneToPresentKeys()
{
    if (m_values.isEmpty())
        return;

    const auto present = presentKeys();

    for (auto it = m_values.begin(); it != m_values.end();) {
        if (present.contains(it.key()))
            ++it;
        else
            it = m_values.erase(it);
    }
}

bool RolesOverlayModel::emitOverlayChanged(const QSet<QString>& keys,
                                           const QVector<int>& roles)
{
    if (keys.isEmpty() || m_keyRoleId == -1)
        return false;

    const auto count = rowCount();
    int first = -1, last = -1;

    for (int row = 0; row < count; row++) {
        if (!keys.contains(keyAt(row)))
            continue;

        if (first == -1)
            first = row;
        last = row;
    }

    if (first == -1)
        return false;

    // a single range, so sorting proxies re-sort all affected rows at once
    emit dataChanged(index(first, 0), index(last, 0), roles);
    return true;
}

} // namespace qtmt

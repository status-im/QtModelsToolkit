#include "qtmodelstoolkit/keysetfiltermodel.h"

namespace qtmt {

/*!
    \qmltype KeySetFilterModel
    \inherits QSortFilterProxyModel
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModels
    \brief Filter proxy accepting rows whose key is in a set of keys.

    A row is accepted when the value of its \l keyRoleName role is one of
    \l keys (or is not, when \l invert is set). The keys are kept as a hash
    set, so filtering costs a single lookup per row, with no expression
    evaluated per row. An empty set accepts no rows.

    Assigning \l keys swaps the whole set and re-filters the model in one pass.
    Rows inserted to the source, and rows whose key changes, are filtered as
    they come.

    \qml
    KeySetFilterModel {
        sourceModel: membersModel
        keyRoleName: "pubKey"
        keys: channelMemberKeys
    }
    \endqml
*/
KeySetFilterModel::KeySetFilterModel(QObject* parent)
    : QSortFilterProxyModel{parent}
{
    // Connected first, so the role is resolved again before any consumer of
    // the reset (source reset or source model change) reads the proxy. Reads
    // during the reset notification may have cached the old role.
    connect(this, &QAbstractItemModel::modelReset,
            this, &KeySetFilterModel::invalidateKeyRole);
}

/*!
    \qmlproperty string KeySetFilterModel::keyRoleName

    Name of the source role compared against \l keys. Defaults to \c "key".
    When the source has no such role, no rows are accepted (all rows when
    \l invert is set).
*/
const QString& KeySetFilterModel::keyRoleName() const
{
    return m_keyRoleName;
}

void KeySetFilterModel::setKeyRoleName(const QString& keyRoleName)
{
    if (m_keyRoleName == keyRoleName)
        return;

    beginKeysFilterChange();
    m_keyRoleName = keyRoleName;
    invalidateKeyRole();
    endKeysFilterChange();

    emit keyRoleNameChanged();
}

/*!
    \qmlproperty list<string> KeySetFilterModel::keys

    Keys of the accepted rows. Order and duplicates are irrelevant.
*/
QStringList KeySetFilterModel::keys() const
{
    return m_keyList;
}

void KeySetFilterModel::setKeys(const QStringList& keys)
{
    QSet<QString> set(keys.cbegin(), keys.cend());

    if (m_keys == set)
        return;

    beginKeysFilterChange();
    m_keyList = keys;
    m_keys = std::move(set);
    endKeysFilterChange();

    emit keysChanged();
}

/*!
    \qmlproperty bool KeySetFilterModel::invert

    When set, rows whose key is \e not in \l keys are accepted instead.
*/
bool KeySetFilterModel::invert() const
{
    return m_invert;
}

void KeySetFilterModel::setInvert(bool invert)
{
    if (m_invert == invert)
        return;

    beginKeysFilterChange();
    m_invert = invert;
    endKeysFilterChange();

    emit invertChanged();
}

void KeySetFilterModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    if (this->sourceModel() == sourceModel)
        return;

    for (const auto& connection : std::as_const(m_sourceConnections))
        disconnect(connection);

    m_sourceConnections.clear();

    if (sourceModel != nullptr)
        connectSourceModel(sourceModel);

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool KeySetFilterModel::filterAcceptsRow(int sourceRow,
                                         const QModelIndex& sourceParent) const
{
    const auto role = keyRole();

    if (role == -1)
        return m_invert;

    const auto key = sourceModel()->index(sourceRow, 0, sourceParent)
            .data(role).toString();

    return m_keys.contains(key) != m_invert;
}

void KeySetFilterModel::connectSourceModel(QAbstractItemModel* model)
{
    // Connected before the base class connects its own handlers, so a missing
    // key role is resolved again before the affected rows are filtered.
    const auto invalidateMissingKeyRole = [this] {
        if (m_keyRoleResolved && m_keyRole == -1)
            invalidateKeyRole();
    };

    // A model starting with no roles may define them with its first insertion
    // (QTBUG-57971).
    m_sourceConnections << connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
                                   this, invalidateMissingKeyRole);

    // QML ListModel adds a role on the first setProperty() of it, notifying
    // only dataChanged.
    m_sourceConnections << connect(model, &QAbstractItemModel::dataChanged,
                                   this, invalidateMissingKeyRole);
}

void KeySetFilterModel::beginKeysFilterChange()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
}

void KeySetFilterModel::endKeysFilterChange()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(Direction::Rows);
#else
    invalidateFilter();
#endif
}

void KeySetFilterModel::invalidateKeyRole()
{
    m_keyRole = -1;
    m_keyRoleResolved = false;
}

int KeySetFilterModel::keyRole() const
{
    if (!m_keyRoleResolved) {
        const auto model = sourceModel();

        m_keyRole = model == nullptr
                ? -1 : model->roleNames().key(m_keyRoleName.toUtf8(), -1);
        m_keyRoleResolved = true;
    }

    return m_keyRole;
}

} // namespace qtmt

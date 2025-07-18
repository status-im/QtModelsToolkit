#include "qtmodelstoolkit/objectproxymodel.h"

#include <QDebug>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>

namespace qtmt {

/*!
    \qmltype ObjectProxyModel
    \inherits QIdentityProxyModel
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModels
    \brief Proxy model using QML delegate to transform source model.

    This proxy creates object from provided delegate component for every row.
    The delegate's instance can access data from source model and expose
    properties as new roles.

    ObjectProxyModel may transform not only plain types but also complex types
    as submodels:

    \qml
    ObjectProxyModel {
        sourceModel: tokensModel

        delegate: SortFilterProxyModel {
            id: delegateRoot

            readonly property int balancesCount: model.balances.ModelCount.count
            readonly property int sum: aggregator.value
            readonly property SortFilterProxyModel balances: this

            sourceModel: model.balances

            filters: ExpressionFilter {
                expression: balance >= thresholdSlider.value
            }

            readonly property SumAggregator aggregator: SumAggregator {
                id: aggregator

                model: delegateRoot
                roleName: "balance"
            }
        }

        exposedRoles: ["balances", "balancesCount", "sum"]
        expectedRoles: ["balances"]
    }
    \endqml

    In this example, submodel "balances" from source model is filtered to keep
    only values higher or equal to a given threshold. Then it's exposed via
    "balances" property, overriding original role. Additionally new roles
    are exposed:
    \list
        \li sum - sum of all balances
        \li balancesCount - row count of the submodel
    \endlist


*/
ObjectProxyModel::ObjectProxyModel(QObject* parent)
    : QIdentityProxyModel{parent}
{
}

QVariant ObjectProxyModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    if (m_delegate && m_exposedRolesSet.contains(role)) {
        const auto proxy = this->proxyObject(index.row());
        return proxy->property(m_roleNames[role]);
    }

    return QIdentityProxyModel::data(index, role);
}

void ObjectProxyModel::setSourceModel(QAbstractItemModel* model)
{
    if (sourceModel() == model)
        return;

    if (sourceModel() != nullptr)
        sourceModel()->disconnect(this);

    m_container.setModel(model);

    if (model == nullptr) {
        QIdentityProxyModel::setSourceModel(nullptr);
        return;
    }

    connect(model, &QAbstractItemModel::rowsMoved, this, [this] {
        updateIndexes(0, m_container.size() - 1);
    });

    connect(model, &QAbstractItemModel::layoutChanged, this, [this] {
        updateIndexes(0, m_container.size() - 1);
    });

    connect(model, &QAbstractItemModel::rowsRemoved, this,
            [this](const QModelIndex& /*parent*/, int first, int /*last*/) {
        Q_ASSERT(first >= 0);

        if (static_cast<std::size_t>(first) >= m_container.size())
            return;

        Q_ASSERT(m_container.size() > 0);
        auto updateLast = m_container.size() - 1;

        if (static_cast<std::size_t>(first) <= updateLast)
            updateIndexes(first, updateLast);
    });

    connect(model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex& /*parent*/, int first, int /*last*/) {
        updateIndexes(first, m_container.size() - 1);
    });

    connect(model, &QAbstractItemModel::dataChanged, this,
            [this, model](const QModelIndex& topLeft,
                const QModelIndex& bottomRight, const auto& /*roles*/)
    {
        if (!topLeft.isValid() || !bottomRight.isValid())
            return;

        auto first = topLeft.row();
        auto last = bottomRight.row();

        for (auto idx = first; idx <= last; idx++) {
            auto rowData = m_container[idx].rowData;

            if (rowData == nullptr)
                continue;

            QHashIterator i(m_expectedRoleNames);
            while (i.hasNext()) {
                i.next();

                rowData->insert(i.value(),
                                model->data(model->index(idx, 0), i.key()));
            }
        }
    });

    // Workaround for QTBUG-57971
    if (model->roleNames().isEmpty())
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &ObjectProxyModel::initRoles);

    QIdentityProxyModel::setSourceModel(model);
}

QHash<int, QByteArray> ObjectProxyModel::roleNames() const
{
    return m_roleNames.isEmpty() && sourceModel() ? sourceModel()->roleNames() : m_roleNames;
}

/*!
    \qmlproperty any ObjectProxyModel::delegate

    This property holds the component created for every row of the source model
    in order to transform the source data in-place, overriding existing roles,
    or/and by exposing new roles.
*/
QQmlComponent* ObjectProxyModel::delegate() const
{
    return m_delegate;
}

void ObjectProxyModel::setDelegate(QQmlComponent* delegate)
{
    if (m_delegate == delegate)
        return;

    m_delegate = delegate;
    emit delegateChanged();

    emitAllDataChanged();
}

void ObjectProxyModel::setExpectedRoles(const QStringList& expectedRoles)
{
    if (m_expectedRoles == expectedRoles)
        return;

    bool hasSource = sourceModel() != nullptr;

    if (hasSource)
        beginResetModel();

    m_expectedRoles = expectedRoles;

    if (hasSource)
        endResetModel();

    emit expectedRolesChanged();
}

/*!
    \qmlproperty list<string> ObjectProxyModel::expectedRoles

    This property holds the names of roles that are intended to be available
    within the delegate's instances.
*/
const QStringList& ObjectProxyModel::expectedRoles() const
{
    return m_expectedRoles;
}

void ObjectProxyModel::setExposedRoles(const QStringList& exposedRoles)
{
    if (m_exposedRoles == exposedRoles)
        return;

    bool hasSource = sourceModel() != nullptr;

    if (hasSource)
        beginResetModel();

    m_exposedRoles = exposedRoles;

    if (hasSource)
        endResetModel();

    emit exposedRolesChanged();
}

/*!
    \qmlproperty list<string> ObjectProxyModel::exposedRoles

    This property holds the names of roles that are exposed from the model.
    Those names must match names of properties defined in a delegate, as those
    properties are data source of those roles.
*/
const QStringList& ObjectProxyModel::exposedRoles() const
{
    return m_exposedRoles;
}

QObject* ObjectProxyModel::proxyObject(int index) const
{
    if (index < 0 || static_cast<std::size_t>(index) >= m_container.size())
        return nullptr;

    auto& entry = m_container[index];

    if (entry.proxy)
        return entry.proxy.get();

    auto creationContext = m_delegate->creationContext();
    auto parentContext = creationContext
            ? creationContext : m_delegate->engine()->rootContext();

    auto context = new QQmlContext(parentContext);
    auto rowData = new QQmlPropertyMap(context);
    auto model = sourceModel();

    QHashIterator i(m_expectedRoleNames);
    while (i.hasNext()) {
        i.next();
        rowData->insert(i.value(), model->data(model->index(index, 0), i.key()));
    }

    rowData->insert(QStringLiteral("index"), index);
    context->setContextProperty(QStringLiteral("model"), rowData);

    QObject* instance = m_delegate->create(context);
    context->setParent(instance);

    for (auto& exposedRole : m_exposedRoles) {
        QQmlProperty prop(instance, exposedRole, m_delegate->engine());

        prop.connectNotifySignal(const_cast<ObjectProxyModel*>(this),
                                 SLOT(onCustomRoleChanged()));
    }

    entry.proxy.reset(instance);
    entry.context = context;
    entry.rowData = rowData;

    return instance;
}

void ObjectProxyModel::resetInternalData()
{
    QIdentityProxyModel::resetInternalData();
    updateRoleNames();

    m_dataChangedQueued = false;
}

void ObjectProxyModel::onCustomRoleChanged()
{
    if (!m_dataChangedQueued) {
        m_dataChangedQueued = true;
        QMetaObject::invokeMethod(this, "emitAllDataChanged",
                                  Qt::QueuedConnection);
    }
}

void ObjectProxyModel::emitAllDataChanged()
{
    m_dataChangedQueued = false;
    auto count = rowCount();

    if (count == 0)
        return;

    if (m_exposedRolesSet.isEmpty())
        return;

    QVector<int> roles(m_exposedRolesSet.cbegin(),
                       m_exposedRolesSet.cend());

    emit this->dataChanged(index(0, 0), index(count - 1, 0), roles);
}

void ObjectProxyModel::initRoles()
{
    disconnect(sourceModel(), &QAbstractItemModel::rowsInserted,
            this, &ObjectProxyModel::initRoles);

    resetInternalData();
}

void ObjectProxyModel::updateRoleNames()
{
    m_roleNames.clear();

    if (sourceModel() == nullptr)
        return;

    auto roles = sourceModel()->roleNames();

    if (roles.empty())
        return;

    m_expectedRoleNames = findExpectedRoles(roles, m_expectedRoles);

    const auto keys = roles.keys();
    const auto maxElementIt = std::max_element(keys.begin(), keys.end());

    Q_ASSERT(maxElementIt != keys.end());

    auto maxRoleKey = *maxElementIt;

    for (auto& exposedRole : std::as_const(m_exposedRoles)) {

        auto exposedRoleByteArray = exposedRole.toUtf8();
        auto keys = roles.keys(exposedRoleByteArray);

        if (!keys.empty()) {
            auto key = keys.first();

            m_exposedRolesSet.insert(key);

            continue;
        }

        auto newRole = ++maxRoleKey;

        roles.insert(newRole, exposedRoleByteArray);
        m_exposedRolesSet.insert(newRole);
    }

    m_roleNames = roles;
}

void ObjectProxyModel::updateIndexes(int from, int to)
{
    for (auto i = from; i <= to; i++) {
        auto& entry = m_container[i];

        if (entry.proxy)
            entry.rowData->insert(QStringLiteral("index"), i);
    }
}

QHash<int, QByteArray> ObjectProxyModel::findExpectedRoles(
        const QHash<int, QByteArray> &roleNames,
        const QStringList &expectedRoles)
{
    if (roleNames.isEmpty() || expectedRoles.isEmpty())
        return {};

    QHash<int, QByteArray> expected;

    for (auto &role : expectedRoles) {
        auto expectedKeys = roleNames.keys(role.toUtf8());
        auto expectedKeysCount = expectedKeys.size();

        if (expectedKeysCount == 1)
            expected.insert(expectedKeys.first(), role.toUtf8());
        else if (expectedKeysCount == 0) {
            qWarning() << Q_FUNC_INFO;
            qWarning() << "Expected role" << role << "not found!";
        } else {
            qWarning() << Q_FUNC_INFO;
            qWarning()
                << "Malformed source model - multiple roles found for given expected role name!";
            return {};
        }
    }

    return expected;
}

} // namespace qtmt

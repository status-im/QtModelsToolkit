#include "qtmodelstoolkit/reverseproxymodel.h"

#include <QDebug>

namespace qtmt {

/*!
    \qmltype ReverseProxyModel
    \inherits QAbstractProxyModel
    \inqmlmodule QtModelsToolkit
    \ingroup ProxyModels
    \brief Proxy model presenting rows of the source model in reverse order.
*/
ReverseProxyModel::ReverseProxyModel(QObject* parent)
    : QAbstractProxyModel{parent}
{
}

void ReverseProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    auto* currentModel = this->sourceModel();

    if (currentModel == sourceModel)
        return;

    beginResetModel();

    if (currentModel != nullptr)
        disconnect(currentModel, nullptr, this, nullptr);

    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel != nullptr)
        connectSourceModel(sourceModel);

    endResetModel();
}

QModelIndex ReverseProxyModel::index(int row, int column,
                                     const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || row >= rowCount()
            || column < 0 || column >= columnCount())
        return {};

    return createIndex(row, column);
}

QModelIndex ReverseProxyModel::parent(const QModelIndex& child) const
{
    Q_UNUSED(child)

    return {};
}

int ReverseProxyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return sourceRowCount();
}

int ReverseProxyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    auto* model = sourceModel();

    return model == nullptr ? 0 : model->columnCount();
}

QModelIndex ReverseProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    auto* model = sourceModel();

    if (model == nullptr || !proxyIndex.isValid())
        return {};

    return model->index(sourceRowCount() - 1 - proxyIndex.row(),
                        proxyIndex.column());
}

QModelIndex ReverseProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (sourceModel() == nullptr || !sourceIndex.isValid())
        return {};

    return createIndex(sourceRowCount() - 1 - sourceIndex.row(),
                       sourceIndex.column());
}

QHash<int, QByteArray> ReverseProxyModel::roleNames() const
{
    auto* model = sourceModel();

    return model == nullptr ? QHash<int, QByteArray>{} : model->roleNames();
}

void ReverseProxyModel::connectSourceModel(QAbstractItemModel* model)
{
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this,
            &ReverseProxyModel::onRowsAboutToBeInserted);

    connect(model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex& parent) {
        if (!parent.isValid())
            endInsertRows();
    });

    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this,
            &ReverseProxyModel::onRowsAboutToBeRemoved);

    connect(model, &QAbstractItemModel::rowsRemoved, this,
            [this](const QModelIndex& parent) {
        if (!parent.isValid())
            endRemoveRows();
    });

    connect(model, &QAbstractItemModel::rowsAboutToBeMoved, this,
            &ReverseProxyModel::onRowsAboutToBeMoved);

    connect(model, &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex& parent, int, int,
                   const QModelIndex& destination) {
        if (!parent.isValid() && !destination.isValid())
            endMoveRows();
    });

    connect(model, &QAbstractItemModel::dataChanged, this,
            &ReverseProxyModel::onDataChanged);

    connect(model, &QAbstractItemModel::modelAboutToBeReset, this,
            [this] { beginResetModel(); });

    connect(model, &QAbstractItemModel::modelReset, this,
            [this] { endResetModel(); });

    connect(model, &QAbstractItemModel::layoutAboutToBeChanged, this,
            &ReverseProxyModel::onLayoutAboutToBeChanged);

    connect(model, &QAbstractItemModel::layoutChanged, this,
            &ReverseProxyModel::onLayoutChanged);
}

int ReverseProxyModel::sourceRowCount() const
{
    auto* model = sourceModel();

    return model == nullptr ? 0 : model->rowCount();
}

void ReverseProxyModel::onRowsAboutToBeInserted(const QModelIndex& parent,
                                                int first, int last)
{
    if (parent.isValid()) {
        qWarning() << "ReverseProxyModel: only flat models are supported";
        return;
    }

    // The source hasn't inserted yet, so the count it reports is still the old
    // one. Every row the insertion pushes along keeps the proxy row it had: a
    // row at source position s >= first ends up at s + count, and
    // (newCount - 1) - (s + count) is exactly its previous proxy row. So the
    // inserted rows are the only ones taking up space, and they take it in one
    // block, mirrored to the other side of the model.
    const auto count = last - first + 1;
    const auto newRowCount = sourceRowCount() + count;

    beginInsertRows({}, newRowCount - 1 - last, newRowCount - 1 - first);
}

void ReverseProxyModel::onRowsAboutToBeRemoved(const QModelIndex& parent,
                                               int first, int last)
{
    if (parent.isValid()) {
        qWarning() << "ReverseProxyModel: only flat models are supported";
        return;
    }

    // Mirrored against the count as it is now, before the removal lands.
    const auto rowCount = sourceRowCount();

    beginRemoveRows({}, rowCount - 1 - last, rowCount - 1 - first);
}

void ReverseProxyModel::onRowsAboutToBeMoved(
        const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex& destinationParent, int destinationRow)
{
    if (sourceParent.isValid() || destinationParent.isValid()) {
        qWarning() << "ReverseProxyModel: only flat models are supported";
        return;
    }

    // The moved block stays contiguous when reversed. The destination - the row
    // the block is inserted before, expressed in pre-move numbering - comes out
    // as count - destinationRow, for a move in either direction. Qt rejects a
    // destination inside [start, end + 1] as a no-op; that range maps onto
    // itself here, so a move the source accepted stays acceptable.
    const auto rowCount = sourceRowCount();

    beginMoveRows({}, rowCount - 1 - sourceEnd, rowCount - 1 - sourceStart,
                  {}, rowCount - destinationRow);
}

void ReverseProxyModel::onDataChanged(const QModelIndex& topLeft,
                                      const QModelIndex& bottomRight,
                                      const QVector<int>& roles)
{
    if (!topLeft.isValid() || !bottomRight.isValid()
            || topLeft.parent().isValid())
        return;

    // Reversal turns the range inside out: the range's last source row is its
    // first proxy row.
    emit dataChanged(mapFromSource(bottomRight), mapFromSource(topLeft), roles);
}

void ReverseProxyModel::onLayoutAboutToBeChanged(
        const QList<QPersistentModelIndex>& parents,
        QAbstractItemModel::LayoutChangeHint hint)
{
    if (!parents.isEmpty()) {
        qWarning() << "ReverseProxyModel: only flat models are supported";
        return;
    }

    emit layoutAboutToBeChanged({}, hint);

    const auto proxyIndexes = persistentIndexList();

    m_layoutChangeProxyIndexes.clear();
    m_layoutChangeSourceIndexes.clear();
    m_layoutChangeProxyIndexes.reserve(proxyIndexes.size());
    m_layoutChangeSourceIndexes.reserve(proxyIndexes.size());

    // Remembered as persistent source indexes, so they follow the items the
    // source is about to shuffle. What they end up pointing at is reversed
    // again once it has settled.
    for (const auto& proxyIndex : proxyIndexes) {
        m_layoutChangeProxyIndexes.append(proxyIndex);
        m_layoutChangeSourceIndexes.append(mapToSource(proxyIndex));
    }
}

void ReverseProxyModel::onLayoutChanged(
        const QList<QPersistentModelIndex>& parents,
        QAbstractItemModel::LayoutChangeHint hint)
{
    if (!parents.isEmpty())
        return;

    for (int i = 0; i < m_layoutChangeProxyIndexes.size(); ++i) {
        changePersistentIndex(m_layoutChangeProxyIndexes.at(i),
                              mapFromSource(m_layoutChangeSourceIndexes.at(i)));
    }

    m_layoutChangeProxyIndexes.clear();
    m_layoutChangeSourceIndexes.clear();

    emit layoutChanged({}, hint);
}

} // namespace qtmt

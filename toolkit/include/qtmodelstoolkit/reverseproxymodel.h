#pragma once

#include <QAbstractProxyModel>

namespace qtmt {

/**
 * @brief Presents a flat source model in reverse order: proxy row 0 is the
 * last source row.
 *
 * The mapping is arithmetic (N - 1 - row), so no order is stored and nothing is
 * recomputed when the source changes. Every structural change is translated
 * into its exact counterpart instead of a reset, which keeps the proxy usable
 * by consumers tracking rows incrementally:
 *
 *  - a row inserted at the source's front is an append here, and vice versa,
 *  - a contiguous source range stays contiguous after reversal,
 *  - persistent indexes survive insertions, removals, moves and source layout
 *    changes.
 *
 * Only flat (list) models are supported; a change under a valid parent is
 * rejected with a warning. Columns are not reversed and column changes are not
 * translated, which list models never make.
 */
class ReverseProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    explicit ReverseProxyModel(QObject* parent = nullptr);

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    void connectSourceModel(QAbstractItemModel* model);

    // Row count as the source reports it right now. The translations below are
    // expressed against the count at the moment they run, which is why an
    // insertion has to be computed in the *about to* slot, before it moves.
    int sourceRowCount() const;

    void onRowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void onRowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart,
                              int sourceEnd, const QModelIndex& destinationParent,
                              int destinationRow);
    // QVector rather than QList: Qt 5 declares dataChanged()'s roles as
    // QVector<int>, and in Qt 6 QVector *is* QList, so this one spelling
    // matches the signal on both - as the other proxies here do.
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                       const QVector<int>& roles);
    void onLayoutAboutToBeChanged(const QList<QPersistentModelIndex>& parents,
                                  QAbstractItemModel::LayoutChangeHint hint);
    void onLayoutChanged(const QList<QPersistentModelIndex>& parents,
                         QAbstractItemModel::LayoutChangeHint hint);

    // Kept between layoutAboutToBeChanged and layoutChanged, so persistent
    // indexes can be re-pointed once the source has settled.
    QList<QPersistentModelIndex> m_layoutChangeSourceIndexes;
    QModelIndexList m_layoutChangeProxyIndexes;
};

} // namespace qtmt

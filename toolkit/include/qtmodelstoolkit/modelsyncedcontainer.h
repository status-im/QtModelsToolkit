#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QVariant>

#include <memory>

namespace qtmt {

template<typename T>
class ModelSyncedContainer
{
public:
    void setModel(QAbstractItemModel* model)
    {
        m_container.clear();

        // create context object for connections, disconnect from previous model
        // by destroying previous context object if present
        m_ctx = std::make_unique<QObject>();

        if (model == nullptr)
            return;

        m_container.resize(model->rowCount());

        QObject::connect(model, &QAbstractItemModel::rowsRemoved, m_ctx.get(),
                         [this] (const QModelIndex& parent, int first, int last)
        {
            if (parent.isValid())
                return;

            m_container.erase(m_container.cbegin() + first,
                              m_container.cbegin() + last + 1);
        });

        QObject::connect(model, &QAbstractItemModel::rowsInserted, m_ctx.get(),
                         [this] (const QModelIndex& parent, int first, int last)
        {
            if (parent.isValid())
                return;

            auto count = last - first + 1;

            if (count <= 0)
                return;

            std::vector<T> toBeInserted(count);
            m_container.insert(m_container.cbegin() + first,
                               std::make_move_iterator(toBeInserted.begin()),
                               std::make_move_iterator(toBeInserted.end()));
        });

        QObject::connect(model, &QAbstractItemModel::rowsAboutToBeMoved,
                         m_ctx.get(), [this, model] (const QModelIndex& parent)
        {
            if (parent.isValid())
                return;

            storePersistentIndexes(model);
        });

        QObject::connect(model, &QAbstractItemModel::rowsMoved,
                         m_ctx.get(), [this, model] (const QModelIndex& parent)
        {
            if (parent.isValid())
                return;

            // This implementation is simplified. Can be replaced by faster
            // implementation not using persistent indexes but reordering
            // the container directly
            updateFromPersistentIndexes(model);
        });

        QObject::connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
                         m_ctx.get(), [this, model]
        {
            storePersistentIndexes(model);
        });

        QObject::connect(model, &QAbstractItemModel::layoutChanged,
                         m_ctx.get(), [this, model]
        {
            updateFromPersistentIndexes(model);
        });

        QObject::connect(model, &QAbstractItemModel::modelReset,
                         m_ctx.get(), [this, model]
        {
            m_container.clear();
            m_container.resize(model->rowCount());
        });

        QObject::connect(model, &QAbstractItemModel::destroyed,
                         m_ctx.get(), [this]
        {
            m_container.clear();
        });
    }

    const T& operator[](std::size_t i) const
    {
        return m_container[i];
    }

    T& operator[](std::size_t i)
    {
        return m_container[i];
    }

    std::size_t size() const {
        return m_container.size();
    }

    const std::vector<T>& data() const {
        return m_container;
    }

private:
    void storePersistentIndexes(QAbstractItemModel* model)
    {
        auto count = model->rowCount();
        m_persistentIndexes.clear();
        m_persistentIndexes.reserve(count);

        for (auto i = 0; i < count; i++)
            m_persistentIndexes.push_back(model->index(i, 0));
    }

    void updateFromPersistentIndexes(QAbstractItemModel* model)
    {
        // The new container must be sized from the model, not from the number of
        // surviving indexes: a layout change may also *add* rows (e.g.
        // QSortFilterProxyModel::invalidate() after a filter loosens), in which case a
        // surviving index can land beyond the count of survivors.
        std::vector<T> newContainer(model == nullptr ? 0 : model->rowCount());

        const int oldCount = static_cast<int>(m_container.size());
        const int newCount = static_cast<int>(newContainer.size());

        for (int i = 0; i < m_persistentIndexes.size(); i++) {
            const QModelIndex idx = m_persistentIndexes[i];

            if (!idx.isValid())
                continue;

            const int row = idx.row();

            if (i >= oldCount || row < 0 || row >= newCount)
                continue;

            newContainer[row] = std::move(m_container[i]);
        }

        std::swap(m_container, newContainer);
        m_persistentIndexes.clear();
    }

    QList<QPersistentModelIndex> m_persistentIndexes;
    std::vector<T> m_container;

    std::unique_ptr<QObject> m_ctx;
};

} // namespace qtmt

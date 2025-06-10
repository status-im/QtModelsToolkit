#pragma once

#include <QAbstractItemModel>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QStringList>

namespace qtmt {

class ModelEntry : public QObject
{
    Q_OBJECT
    ////////////// input
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged REQUIRED)
    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged REQUIRED)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged REQUIRED)
    Q_PROPERTY(bool cacheOnRemoval READ cacheOnRemoval WRITE setCacheOnRemoval NOTIFY cacheOnRemovalChanged)

    ///////////// output
    Q_PROPERTY(QQmlPropertyMap* item READ item NOTIFY itemChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QStringList roles READ roles NOTIFY rolesChanged)
    Q_PROPERTY(int row READ row NOTIFY rowChanged)
    Q_PROPERTY(bool itemRemovedFromModel READ itemRemovedFromModel NOTIFY itemRemovedFromModelChanged)

public:
    explicit ModelEntry(QObject* parent = nullptr);

    QAbstractItemModel* sourceModel() const;
    QString key() const;
    QVariant value() const;
    bool cacheOnRemoval() const;

    QQmlPropertyMap* item() const;
    bool available() const;
    const QStringList& roles() const;
    int row() const;
    bool itemRemovedFromModel() const;

protected:
    void setSourceModel(QAbstractItemModel* sourceModel);
    void setKey(const QString& key);
    void setValue(const QVariant& value);
    void setIndex(const QModelIndex& index);
    void setAvailable(bool available);
    void setRoles(const QStringList& roles);
    void setRow(int row);
    void setCacheOnRemoval(bool cacheOnRemoval);
    void setItemRemovedFromModel(bool itemRemovedFromModel);

    void resetIndex();
    void tryItemResetOrUpdate();
    void resetItem();
    void updateItem(const QVector<int>& roles = {});
    QStringList fillItem(const QVector<int>& roles = {});
    void notifyItemChanges(const QStringList& roles);

    QModelIndex findIndexInRange(int start, int end) const;
    bool itemHasCorrectRoles() const;
    void cacheItem();
    void resetCachedItem();

signals:
    void sourceModelChanged();
    void keyChanged();
    void valueChanged();
    void itemChanged();
    void availableChanged();
    void rolesChanged();
    void rowChanged();
    void cacheOnRemovalChanged();
    void itemRemovedFromModelChanged();

private:
    QScopedPointer<QQmlPropertyMap> m_item{nullptr};
    QPointer<QAbstractItemModel> m_sourceModel{nullptr};
    QPersistentModelIndex m_index;
    bool m_available{false};
    QStringList m_roles;
    int m_row{-1};
    bool m_cacheOnRemoval{false};
    bool m_itemRemovedFromModel{false};
    QVariant m_value;
    QString m_key;
};

} // namespace qtmt

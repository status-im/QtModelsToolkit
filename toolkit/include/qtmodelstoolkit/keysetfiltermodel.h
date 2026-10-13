#pragma once

#include <QSet>
#include <QSortFilterProxyModel>
#include <QStringList>

namespace qtmt {

/**
 * @brief Filter proxy accepting rows whose key role value is in a set of keys.
 *
 * The set is kept as a hash set, so filtering costs one lookup per row and no
 * expression is evaluated. The key role is resolved once per source model or
 * roles change. Setting keys swaps the whole set and re-filters in one pass.
 */
class KeySetFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString keyRoleName READ keyRoleName WRITE setKeyRoleName
               NOTIFY keyRoleNameChanged)
    Q_PROPERTY(QStringList keys READ keys WRITE setKeys NOTIFY keysChanged)
    Q_PROPERTY(bool invert READ invert WRITE setInvert NOTIFY invertChanged)

public:
    explicit KeySetFilterModel(QObject* parent = nullptr);

    const QString& keyRoleName() const;
    void setKeyRoleName(const QString& keyRoleName);

    QStringList keys() const;
    void setKeys(const QStringList& keys);

    bool invert() const;
    void setInvert(bool invert);

    void setSourceModel(QAbstractItemModel* sourceModel) override;

signals:
    void keyRoleNameChanged();
    void keysChanged();
    void invertChanged();

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex& sourceParent) const override;

private:
    void connectSourceModel(QAbstractItemModel* model);

    // begin/endFilterChange() where available (Qt 6.10+, invalidateFilter() is
    // deprecated there), invalidateFilter() otherwise
    void beginKeysFilterChange();
    void endKeysFilterChange();
    void invalidateKeyRole();
    int keyRole() const;

    QString m_keyRoleName = QStringLiteral("key");
    QStringList m_keyList;
    QSet<QString> m_keys;
    bool m_invert = false;

    QList<QMetaObject::Connection> m_sourceConnections;

    // resolved lazily, on the first filtering after a source or roles change
    mutable int m_keyRole = -1;
    mutable bool m_keyRoleResolved = false;
};

} // namespace qtmt

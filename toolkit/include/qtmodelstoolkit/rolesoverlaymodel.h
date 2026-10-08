#pragma once

#include <QHash>
#include <QIdentityProxyModel>
#include <QSet>
#include <QVariantMap>

namespace qtmt {

/**
 * @brief Identity proxy adding writable roles, addressed by a key role.
 *
 * Roles listed in defaults are added to (or override) the source roles. Values
 * written via set() are stored per key, not per row, so no per-row objects or
 * state are created and sorting/filtering on those roles stays cheap. Values
 * are kept only for keys present in the source: they follow rows when moved,
 * and are dropped once the key is gone after a removal, reset or layout
 * change. Keys are expected to be unique.
 */
class RolesOverlayModel : public QIdentityProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString keyRole READ keyRole WRITE setKeyRole NOTIFY keyRoleChanged)
    Q_PROPERTY(QVariantMap defaults READ defaults WRITE setDefaults NOTIFY defaultsChanged)

public:
    explicit RolesOverlayModel(QObject* parent = nullptr);

    const QString& keyRole() const;
    void setKeyRole(const QString& keyRole);

    const QVariantMap& defaults() const;
    void setDefaults(const QVariantMap& defaults);

    Q_INVOKABLE void set(const QString& key, const QString& role, const QVariant& value);
    Q_INVOKABLE QVariant get(const QString& key, const QString& role) const;

    Q_INVOKABLE QVariantList entries() const;
    Q_INVOKABLE void setEntries(const QVariantList& entries);
    Q_INVOKABLE void clear();

    void setSourceModel(QAbstractItemModel* sourceModel) override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;

signals:
    void keyRoleChanged();
    void defaultsChanged();

protected slots:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void resetInternalData() override;
#else
    void resetInternalData();
#endif

private:
    void updateRoleNames();
    QString keyAt(int row) const;
    QSet<QString> presentKeys() const;
    void pruneToPresentKeys();

    // returns whether any row with one of the keys was found
    bool emitOverlayChanged(const QSet<QString>& keys, const QVector<int>& roles);

    QString m_keyRole;
    QVariantMap m_defaults;

    QHash<int, QByteArray> m_roleNames;
    QHash<int, QString> m_overlayRoles;
    int m_keyRoleId = -1;

    QHash<QString, QVariantMap> m_values;
    QSet<QString> m_removedKeys;
};

} // namespace qtmt

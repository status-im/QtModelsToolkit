#pragma once

#include <QAbstractListModel>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <QPointer>

#include <unordered_map>

namespace qtmt {

class SourceModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* model READ model
                   WRITE setModel NOTIFY modelChanged)

    Q_PROPERTY(QVariant markerRoleValue READ markerRoleValue
                   WRITE setMarkerRoleValue NOTIFY markerRoleValueChanged)

public:
    explicit SourceModel(QObject* parent = nullptr);

    void setModel(QAbstractItemModel* model);
    QAbstractItemModel* model() const;

    void setMarkerRoleValue(const QVariant& markerRoleValue);
    const QVariant& markerRoleValue() const;

signals:
    void modelAboutToBeChanged();
    void modelChanged();
    void markerRoleValueChanged();

private:
    QPointer<QAbstractItemModel> m_model;
    QVariant m_markerRoleValue;
};

class ConcatModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQmlListProperty<qtmt::SourceModel> sources READ sources CONSTANT)

    Q_PROPERTY(QString markerRoleName READ markerRoleName
                   WRITE setMarkerRoleName NOTIFY markerRoleNameChanged)

    Q_PROPERTY(QStringList expectedRoles READ expectedRoles
                   WRITE setExpectedRoles NOTIFY expectedRolesChanged)

    Q_PROPERTY(bool propagateResets READ propagateResets
                   WRITE setPropagateResets NOTIFY propagateResetsChanged)
public:
    explicit ConcatModel(QObject *parent = nullptr);

    QQmlListProperty<SourceModel> sources();

    void setMarkerRoleName(const QString& markerRoleName);
    const QString& markerRoleName() const;

    void setExpectedRoles(const QStringList& expectedRoles);
    const QStringList& expectedRoles() const;

    void setPropagateResets(bool propagateResets);
    bool propagateResets() const;

    Q_INVOKABLE int sourceModelRow(int row) const;
    Q_INVOKABLE QAbstractItemModel* sourceModel(int row) const;
    Q_INVOKABLE int fromSourceRow(const QAbstractItemModel* model, int row) const;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    // QQmlParserStatus interface
    void classBegin() override;
    void componentComplete() override;

signals:
    void markerRoleNameChanged();
    void expectedRolesChanged();
    void propagateResetsChanged();

private:
    static constexpr auto s_defaultMarkerRoleName = "whichModel";

    std::pair<SourceModel*, int> sourceForIndex(int index) const;

    void initRoles();

    void initRolesMapping(int index, QAbstractItemModel* model);
    void initRolesMapping();

    void initAllModelsSlots();
    void connectModelSlots(int index, QAbstractItemModel* model);
    void disconnectModelSlots(QAbstractItemModel* model);

    int rowCountInternal() const;
    int countPrefix(int sourceIndex) const;
    void fetchRowCounts();

    QVector<int> mapFromSourceRoles(int sourceIndex,
                                    const QVector<int>& sourceRoles) const;

    QList<SourceModel*> m_sources;
    QStringList m_expectedRoles;
    bool m_propagateResets = false;

    QString m_markerRoleName = s_defaultMarkerRoleName;
    int m_markerRole = 0;

    bool m_initialized = false;

    QHash<int, QByteArray> m_roleNames;
    std::unordered_map<QByteArray, int> m_nameRoles;

    std::vector<std::unordered_map<int, int>> m_rolesMappingFromSource;
    std::vector<std::unordered_map<int, int>> m_rolesMappingToSource;
    std::vector<bool> m_rolesMappingInitializationFlags;
    std::vector<int> m_rowCounts;

    // helpers for handling layoutChanged from source
    QList<QPersistentModelIndex> m_layoutChangePersistentIndexes;
    QModelIndexList m_proxyIndexes;
};

} // namespace qtmt


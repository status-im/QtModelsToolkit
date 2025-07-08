#pragma once

#include <QAbstractListModel>

namespace qtmt {

class TestModel : public QAbstractListModel {

public:
    TestModel() = default;
    explicit TestModel(QList<QPair<QString, QVariantList>> data);
    explicit TestModel(QList<QString> roles);

    int rowCount(const QModelIndex& parent = {}) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void insert(int index, QVariantList row);
    void append(QVariantList row);

    // intened to be used only when the model has no role names specified,
    // to mimic behavior of first insertion/append to an empty ListModel,
    // described in https://bugreports.qt.io/browse/QTBUG-57971. The model emits
    // rowsAboutToBeInserted/rowsInserted with changed roleNames, without
    // issuing model reset.
    void appendAndInitRoles(QList<QPair<QString, QVariantList>> data);

    void update(int index, int role, QVariant value);
    void remove(int index);

    // inverts order of items, emits layoutAboutToBeChanged / layoutChanged
    void invert();

    // removes every second item from the model but doesn't emit
    // rowsAboutToBeRemoved/rowsRemoved. The update is notified via
    // layoutAboutToBeChanged/layoutChanged. It's useful for testing proxy
    // models against that scenario, which may occur in some circumstances, e.g.
    // during SFPM initialization where initial filtering is notified this way.
    void removeEverySecond();

    // emits modelAboutToBeReset/modelReset, content remains the same
    void reset();

    // emits modelAboutToBeReset/modelReset, sets new roles and data
    void reset(QList<QPair<QString, QVariantList>> data);

    // emits modelAboutToBeReset/modelReset, content is removed, roles remain the
    // same
    void resetAndClear();

private:
    void initRoles();

    QList<QPair<QString, QVariantList>> m_data;
    QHash<int, QByteArray> m_roles;
};

} // namespace qtmt

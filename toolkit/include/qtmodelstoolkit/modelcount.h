#pragma once

#include <QObject>
#include <QQmlEngine>

class QAbstractItemModel;

namespace qtmt {

class ModelCount : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)

public:
    explicit ModelCount(QObject* parent = nullptr);

    static ModelCount* qmlAttachedProperties(QObject* object);

    int count() const;
    bool empty() const;

signals:
    void countChanged();
    void emptyChanged();

private:
    int m_count = 0;
};

} // namespace qtmt

QML_DECLARE_TYPEINFO(qtmt::ModelCount, QML_HAS_ATTACHED_PROPERTIES)

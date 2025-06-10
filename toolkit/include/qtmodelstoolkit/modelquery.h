#pragma once

#include <QJSValue>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QAbstractItemModel>

namespace qtmt {

class ModelQuery : public QObject
{
    Q_OBJECT

public:
    explicit ModelQuery(QObject* parent = nullptr);

    Q_INVOKABLE bool isModel(const QVariant &obj) const;

    Q_INVOKABLE int roleByName(QAbstractItemModel *model,
                               const QString &roleName) const;

    Q_INVOKABLE QStringList roleNames(QAbstractItemModel *model) const;

    Q_INVOKABLE QVariantMap get(QAbstractItemModel *model, int row) const;
    Q_INVOKABLE QVariant get(QAbstractItemModel *model, int row,
                             const QString &roleName) const;

    Q_INVOKABLE int indexOf(QAbstractItemModel *model,
                            const QString &roleName,
                            const QVariant &value) const;

    Q_INVOKABLE QPersistentModelIndex persistentIndex(QAbstractItemModel* model,
                                                      int index);

    Q_INVOKABLE bool contains(QAbstractItemModel *model, const QString &roleName,
                              const QVariant &value, int mode = Qt::CaseSensitive) const;
};

} // namespace qtmt

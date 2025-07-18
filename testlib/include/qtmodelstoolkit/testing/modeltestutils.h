#pragma once

#include <QAbstractListModel>

namespace qtmt {

bool isSame(const QAbstractItemModel& model1, const QAbstractItemModel& model2,
            bool recursive = true);
bool isSame(const QAbstractItemModel* model1, const QAbstractItemModel* model2,
            bool recursive = true);

bool isNotSame(const QAbstractItemModel& model1, const QAbstractItemModel& model2,
               bool recursive = true);
bool isNotSame(const QAbstractItemModel* model1, const QAbstractItemModel* model2,
               bool recursive = true);

} // namespace qtmt

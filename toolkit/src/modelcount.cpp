#include "qtmodelstoolkit/modelcount.h"

#include <QAbstractItemModel>

namespace qtmt {

/*!
    \qmltype ModelCount
    \inherits QObject
    \inqmlmodule QtModelsToolkit
    \ingroup Utilities
    \brief Attached property exposing model's row count.

    This attached type provides row count in a dynamic way. In opposition
    to rowCount() it can be safely used in binding expression because change will
    be properly notified resulting in expression re-evaluation.
*/
ModelCount::ModelCount(QObject* parent) : QObject(parent)
{
    auto model = qobject_cast<QAbstractItemModel*>(parent);

    if (model == nullptr)
        return;

    m_count = model->rowCount();

    auto update = [this, model] {
        auto wasEmpty = m_count == 0;
        auto count = model->rowCount();

        if (m_count == count)
            return;

        m_count = count;
        emit this->countChanged();

        if (wasEmpty != (count == 0))
            emit this->emptyChanged();
    };

    connect(model, &QAbstractItemModel::rowsInserted, this, update);
    connect(model, &QAbstractItemModel::rowsRemoved, this, update);
    connect(model, &QAbstractItemModel::modelReset, this, update);
    connect(model, &QAbstractItemModel::layoutChanged, this, update);
}

ModelCount* ModelCount::qmlAttachedProperties(QObject* object)
{
    return new ModelCount(object);
}

/*!
    \qmlattachedproperty int ModelCount::count
    This attached property provides row count for arbitrary
    QAbstractItemModel model.
*/
int ModelCount::count() const
{
    return m_count;
}

/*!
    \qmlattachedproperty int ModelCount::empty
    This attached property provides flag indication if given arbitrary
    QAbstractItemModel model is empty.
*/
bool ModelCount::empty() const
{
    return m_count == 0;
}

} // namespace qtmt

#include "qtmodelstoolkit/aggregator.h"
#include <QAbstractItemModel>

namespace qtmt {

/*!
    \qmltype Aggregator
    \inherits QtObject
    \qmlabstract
    \inqmlmodule QtModelsToolkit
    \ingroup Aggregators
    \brief Common interface for all aggregators.
*/
Aggregator::Aggregator(QObject* parent)
    : QObject(parent)
{
    connect(this, &Aggregator::modelChanged, this, &Aggregator::recalculate);
}

/*!
    \qmlproperty any Aggregator::model

    The source model used for aggregation.
*/
QAbstractItemModel* Aggregator::model() const
{
    return m_model;
}

void Aggregator::setModel(QAbstractItemModel* model)
{
    if(m_model == model)
        return;

    if(m_model)
        disconnect(m_model, nullptr, this, nullptr);

    m_model = model;
    connectToModel();
    emit modelChanged();
}

/*!
    \qmlproperty any Aggregator::value

    The value computed from the model.
*/
QVariant Aggregator::value() const
{
    return m_value;
}

void Aggregator::recalculate()
{
    auto newValue = calculateAggregation();

    if (m_value == newValue)
        return;

    m_value = newValue;
    emit valueChanged();
}

void Aggregator:: connectToModel()
{
    if(m_model == nullptr)
        return;

    connect(m_model, &QAbstractItemModel::rowsInserted, this, &Aggregator::recalculate);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &Aggregator::recalculate);
    connect(m_model, &QAbstractItemModel::modelReset, this, &Aggregator::recalculate);

    // Some models (like SFPM) emit layoutAboutToBeChanged/layoutChanged while
    // removing some rows in the meantime. If the row count is changed, then
    // aggregation must be recalculated. In most cases, for regular layout
    // change events it's not necessary to recalculate.
    connect(m_model, &QAbstractItemModel::layoutAboutToBeChanged, this, [this] {
        auto model = this->model();

        if (model == nullptr)
            return;

        m_onLayoutToBeChangedCount = model->rowCount();
    });

    connect(m_model, &QAbstractItemModel::layoutChanged, this, [this] {
        auto model = this->model();

        if (model != nullptr && m_onLayoutToBeChangedCount != model->rowCount())
            this->recalculate();
    });

    connect(m_model, &QAbstractItemModel::dataChanged, this,
          [this](auto&, auto&, const auto& roles) {
      if (this->acceptRoles(roles))
          this->recalculate();
    });
}

} // namespace qtmt

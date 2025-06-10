#include "qtmodelstoolkit/functionaggregator.h"

#include <QDebug>
#include <QJSEngine>

namespace qtmt {

/*!
    \qmltype FunctionAggregator
    \inherits SingleRoleAggregator
    \inqmlmodule QtModelsToolkit
    \ingroup Aggregators
    \brief Aggregator calculating value using provided function.

    In the following example \c symbolsAggregator provides list of symbols
    \c {["SNT", "ETH", "ZRX"]}. The second instance \c symbolsAggregator
    computes maximum value of role \c balance (24).

    \qml
    ListModel {
        id: sourceModel

        ListElement {
            symbol: "SNT"
            balance: "4"
        }
        ListElement {
            symbol: "ETH"
            balance: "14"
        }
        ListElement {
            symbol: "ZRX"
            balance: "24"
        }
    }

    FunctionAggregator {
        id: symbolsAggregator

        model: sourceModel
        initialValue: []
        roleName: "symbol"

        aggregateFunction: (aggr, value) => [...aggr, value]
    }

    FunctionAggregator {
        id: maxBalanceAggregator

        model: sourceModel
        initialValue: "0"
        roleName: "balance"

        aggregateFunction: (aggr, value) => Math.max(aggr, value)
    }
    \endqml
*/
FunctionAggregator::FunctionAggregator(QObject* parent)
    : SingleRoleAggregator(parent)
{
    recalculate();
}

/*!
    \qmlproperty any FunctionAggregator::initialValue

    Initial value used to compute aggregation.
*/
const QVariant& FunctionAggregator::initialValue() const
{
    return m_initialValue;
}

void FunctionAggregator::setInitialValue(const QVariant& initialValue)
{
    if (m_initialValue == initialValue)
        return;

    m_initialValue = initialValue;

    emit initialValueChanged();
    recalculate();
}

/*!
    \qmlproperty any FunctionAggregator::aggregateFunction

    Initial value used to compute aggregation.
*/
const QJSValue& FunctionAggregator::aggregateFunction() const
{
    return m_aggregateFunction;
}

void FunctionAggregator::setAggregateFunction(const QJSValue& aggregateFunction)
{
    if (m_aggregateFunction.strictlyEquals(aggregateFunction))
        return;

    if (!aggregateFunction.isCallable() && !aggregateFunction.isUndefined()) {
        qWarning() << "FunctionAggregator::aggregateFunction must be a "
                      "callable object.";
        return;
    }

    m_aggregateFunction = aggregateFunction;

    emit aggregateFunctionChanged();
    recalculate();
}

QVariant FunctionAggregator::calculateAggregation()
{
    // Check if m_model exists and role name is initialized
    if (!model() || roleName().isEmpty())
        return m_initialValue;

    // Check if m_roleName is part of the roles of the model
    QHash<int, QByteArray> roles = model()->roleNames();
    if (!roleExists() && model()->rowCount()) {
        qWarning() << "Provided role name does not exist in the current model.";
        return m_initialValue;
    }

    if (!m_initialValue.isValid())
        return m_initialValue;

    if (m_aggregateFunction.isUndefined())
        return m_initialValue;

    QJSEngine* engine = qjsEngine(this);

    if (engine == nullptr) {
        qWarning() << "FunctionAggregator is intended to be used in JS "
                      "environment. QJSEngine must be available.";
        return m_initialValue;
    }

    QJSValue aggregation = engine->toScriptValue(m_initialValue);

    auto rows = model()->rowCount();
    auto role = roles.key(roleName());

    for (int i = 0; i < rows; ++i) {
        QModelIndex index = model()->index(i, 0);
        QVariant value = model()->data(index, role);

        QJSValue valueJs = engine->toScriptValue(value);

        aggregation = m_aggregateFunction.call({aggregation, valueJs});

        if (aggregation.isError()) {
            qWarning() << "Aggregation calculation failed. Error type:"
                       << aggregation.errorType();
            return m_initialValue;
        }
    }

    return aggregation.toVariant();
}

} // namespace qtmt

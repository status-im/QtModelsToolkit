#pragma once

#include <QObject>
#include <QVariant>

#include "qtmodelstoolkit/singleroleaggregator.h"

namespace qtmt {

class SumAggregator : public SingleRoleAggregator {

public:
    explicit SumAggregator(QObject *parent = nullptr);

protected slots:
     QVariant calculateAggregation() override;
};

} // namespace qtmt

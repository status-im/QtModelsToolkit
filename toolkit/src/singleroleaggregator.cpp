#include "qtmodelstoolkit/singleroleaggregator.h"

namespace qtmt {

/*!
    \qmltype SingleRoleAggregator
    \qmlabstract
    \inherits Aggregator
    \inqmlmodule QtModelsToolkit
    \ingroup Aggregators
    \brief Common interface for all aggregators operating on a single role.
*/
SingleRoleAggregator::SingleRoleAggregator(QObject *parent)
    : Aggregator(parent) {}

/*!
    \qmlproperty any SingleRoleAggregator::roleName

    Name of the role used for aggregation.
*/
const QByteArray& SingleRoleAggregator::roleName() const {
    return m_roleName;
}

void SingleRoleAggregator::setRoleName(const QByteArray &roleName) {
    if (m_roleName == roleName)
        return;
    m_roleName = roleName;
    emit roleNameChanged();

    recalculate();
}

bool SingleRoleAggregator::acceptRoles(const QVector<int>& roles) {
   QHash<int, QByteArray> roleNames = model()->roleNames();
   for (const int role : roles) {
       if (roleNames.contains(role)) {
           QString roleName = QString::fromUtf8(roleNames[role]);

           // Check if the role name is equal to the expected one:
           if (roleName == m_roleName) {
               return true;
           }
       }
   }

   // If we reach this point, none of the roles match m_roleName
   return false;
}

bool SingleRoleAggregator::roleExists() const {
    return !model()->roleNames().keys(m_roleName).empty();
}

} // namespace qtmt

#include "qtmodelstoolkit/testing/persistentindexestester.h"

#include <QDebug>

namespace qtmt {

PersistentIndexesTester::PersistentIndexesTester(QAbstractItemModel* model)
    : m_model(model)
{
    storeIndexesAndData();
}

PersistentIndexesTester::~PersistentIndexesTester() = default;

void PersistentIndexesTester::storeIndexesAndData()
{
    if (m_model == nullptr) {
        qWarning() << "PersistentIndexesTester: model cannot be null.";
        return;
    }

    m_data.clear();

    const auto roles = m_model->roleNames().keys();
    auto count = m_model->rowCount();

    for(auto role : roles)
    {
        for(int i = 0; i < count; i++)
        {
            QVariant data = m_model->data(m_model->index(i, 0), role);
            m_data[role].push_back(data);
        }
    }

    m_persistentIndexes.clear();
    m_persistentIndexes.reserve(count);

    for (auto i = 0; i < count; i++)
        m_persistentIndexes << m_model->index(i, 0);
}

bool PersistentIndexesTester::compare()
{
    if (m_model == nullptr) {
        qWarning() << "PersistentIndexesTester: model cannot be null.";
        return false;
    }

    const auto count = m_model->rowCount();
    const auto roles = m_model->roleNames().keys();

    for (auto i = 0; i < count; i++) {
        const auto idx = m_persistentIndexes[i];

        for (auto role : roles) {
            auto fromIndex = idx.data(role);
            auto fromStash = m_data[role].at(i);

            if (fromIndex != fromStash) {
                auto roleName = QString::fromUtf8(m_model->roleNames()
                                                  .value(role));

                qWarning() << QString("Mismatch detected. Persistent index "
                                      "data: %1, snapshot data: %2, idx: %3, "
                                      "role: %4")
                              .arg(fromIndex.toString(), fromStash.toString(),
                                       QString::number(i), roleName);
                return false;
            }
        }
    }

    return true;
}

} // namespace qtmt

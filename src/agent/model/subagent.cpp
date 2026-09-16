#include "agent/model/subagent.h"

namespace Conversation {
    Subagent::Subagent(const QString &runtimeId, const QString &role, const QString &activity, QObject *parent)
        : QObject(parent), m_runtimeId(runtimeId), m_role(role), m_activity(activity) {
    }

    QString Subagent::runtimeIdGet() const {
        return m_runtimeId;
    }

    QString Subagent::roleGet() const {
        return m_role;
    }

    QString Subagent::activityGet() const {
        return m_activity;
    }

    void Subagent::activitySet(const QString &activity) {
        if (m_activity == activity) return;
        m_activity = activity;
        emit changeActivity();
    }
}

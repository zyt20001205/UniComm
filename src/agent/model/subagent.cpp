#include "agent/model/subagent.h"

namespace Conversation {
    Subagent::Subagent(QObject *parent) : QObject(parent) {
    }

    QString Subagent::typeGet() const {
        return "subagent";
    }

    QVariantList Subagent::actionsGet() const {
        return m_actions;
    }

    int Subagent::countGet() const {
        return static_cast<int>(m_actions.size());
    }

    bool Subagent::expandedGet() const {
        return m_expanded;
    }

    void Subagent::actionAppend(const QString &activity) {
        m_actions.append(activity);
        emit changeActions();
    }

    void Subagent::expandedSet(const bool expanded) {
        if (m_expanded == expanded) return;
        m_expanded = expanded;
        emit changeExpanded();
    }
}

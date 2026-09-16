#include "agent/model/tool.h"

#include "agent/model/message.h"

namespace Conversation {
    Tool::Tool(QObject *parent) : QObject(parent) {
    }

    QString Tool::typeGet() const {
        return "tool";
    }

    QVariantList Tool::messagesGet() const {
        return m_messages;
    }

    int Tool::countGet() const {
        return static_cast<int>(m_messages.size());
    }

    bool Tool::expandedGet() const {
        return m_expanded;
    }

    void Tool::messageAppend(Message *message) {
        m_messages.append(QVariant::fromValue(static_cast<QObject *>(message)));
        emit changeMessages();
    }

    void Tool::expandedSet(const bool expanded) {
        if (m_expanded == expanded) return;
        m_expanded = expanded;
        emit changeExpanded();
    }
}

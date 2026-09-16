#include "agent/model/turn.h"

#include "agent/model/message.h"
#include "agent/model/subagent.h"

namespace Conversation {
    Turn::Turn(const QString &id, const qint64 startedAt, QObject *parent)
        : QObject(parent), m_id(id), m_startedAt(startedAt) {
    }

    QString Turn::idGet() const {
        return m_id;
    }

    double Turn::startedAtGet() const {
        return static_cast<double>(m_startedAt);
    }

    double Turn::finishedAtGet() const {
        return static_cast<double>(m_finishedAt);
    }

    QString Turn::promptGet() const {
        return m_promptChat == nullptr ? QString{} : m_promptChat->contentGet();
    }

    QString Turn::responseGet() const {
        return m_lastAssistant == nullptr ? QString{} : m_lastAssistant->contentGet();
    }

    Message *Turn::promptChatGet() const {
        return m_promptChat;
    }

    bool Turn::collapsedGet() const {
        return m_collapsed;
    }

    QVariantList Turn::blocksGet() const {
        return m_blocks;
    }

    QVariantList Turn::subagentsGet() const {
        return m_subagents;
    }

    QString Turn::activityGet() const {
        return m_activity;
    }

    QUrl Turn::activityIconGet() const {
        return m_activityIcon;
    }

    void Turn::blockAppend(QObject *block) {
        m_blocks.append(QVariant::fromValue(block));
        emit changeBlocks();
    }

    void Turn::subagentAppend(Subagent *subagent) {
        m_subagents.append(QVariant::fromValue(static_cast<QObject *>(subagent)));
        emit changeSubagents();
    }

    void Turn::promptChatSet(Message *chat) {
        m_promptChat = chat;
        connect(chat, &Message::changeContent, this, &Turn::changePrompt);
    }

    void Turn::finishedAtSet(const qint64 finishedAt) {
        m_finishedAt = finishedAt;
        emit changeFinishedAt();
    }

    void Turn::collapsedSet(const bool collapsed) {
        if (m_collapsed == collapsed) return;
        m_collapsed = collapsed;
        emit changeCollapsed();
    }

    void Turn::activitySet(const QString &activity, const QUrl &activityIcon) {
        m_activity = activity;
        m_activityIcon = activityIcon;
        emit changeActivity();
    }

    QString Turn::lastActivityGet() const {
        return m_lastActivity;
    }

    void Turn::lastActivitySet(const QString &activity) {
        m_lastActivity = activity;
    }

    Tool *Turn::toolGet() const {
        return m_tool;
    }

    void Turn::toolSet(Tool *tool) {
        m_tool = tool;
    }

    Message *Turn::lastAssistantGet() const {
        return m_lastAssistant;
    }

    void Turn::lastAssistantSet(Message *assistant) {
        if (m_lastAssistant == assistant) return;
        if (m_lastAssistant != nullptr) disconnect(m_lastAssistant, &Message::changeContent, this, &Turn::changeResponse);
        m_lastAssistant = assistant;
        connect(assistant, &Message::changeContent, this, &Turn::changeResponse);
        emit changeResponse();
    }
}

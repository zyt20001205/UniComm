#include "agent/model/conversationModel.h"

#include "agent/model/message.h"
#include "agent/model/subagent.h"
#include "agent/model/tool.h"
#include "agent/model/turn.h"

namespace Conversation {

Model::Model(QObject *parent) : QAbstractListModel(parent) {
}

int Model::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_turns.size());
}

QVariant Model::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_turns.size()) || role != TurnRole) return {};
    return QVariant::fromValue(static_cast<QObject *>(m_turns.at(index.row())));
}

QHash<int, QByteArray> Model::roleNames() const {
    return {{TurnRole, "turn"}};
}

void Model::clear() {
    beginResetModel();
    qDeleteAll(m_turns);
    m_turns.clear();
    m_turnIndex.clear();
    m_chatIndex.clear();
    m_chatTurns.clear();
    endResetModel();
}

void Model::turnCreate(const QString &turnId, const qint64 startedAt) {
    const auto row = static_cast<int>(m_turns.size());
    beginInsertRows({}, row, row);
    auto *turn = new Turn(turnId, startedAt, this); // NOLINT
    m_turns.append(turn);
    m_turnIndex.insert(turnId, turn);
    endInsertRows();
}

void Model::turnFinish(const QString &turnId, const qint64 finishedAt) {
    auto *turn = turnGet(turnId);
    if (auto *assistant = turn->lastAssistantGet()) assistant->roleSet("assistant");
    turn->finishedAtSet(finishedAt);
    turn->collapsedSet(true);
    turn->activitySet({});
}

void Model::chatCreate(const QString &turnId, const QString &messageId, const QString &role, const QList<QUrl> &attachments) {
    auto *turn = turnGet(turnId);
    auto displayRole = role;
    if (role == "user" && turn->promptChatGet() != nullptr) displayRole = "steering";
    else if (role == "assistant") displayRole = "comment";

    if (displayRole == "tool") {
        if (turn->lastActivityGet() != "tool") {
            auto *tool = new Tool(turn); // NOLINT
            turn->blockAppend(tool);
            turn->toolSet(tool);
        }
        auto *chat = new Message(messageId, displayRole, attachments, turn->toolGet()); // NOLINT
        turn->toolGet()->messageAppend(chat);
        turn->lastActivitySet("tool");
        m_chatIndex.insert(messageId, chat);
        m_chatTurns.insert(messageId, turn);
        return;
    }

    if (displayRole == "steering") turn->lastActivitySet("steering");
    auto *chat = new Message(messageId, displayRole, attachments, turn); // NOLINT
    turn->blockAppend(chat);
    if (displayRole == "user") turn->promptChatSet(chat);
    m_chatIndex.insert(messageId, chat);
    m_chatTurns.insert(messageId, turn);
}

void Model::chatAppend(const QString &messageId, const QString &text) {
    auto *chat = m_chatIndex.value(messageId);
    auto *turn = m_chatTurns.value(messageId);
    chat->contentAppend(text);
    if (chat->roleGet() == "comment") {
        turn->lastAssistantSet(chat);
        turn->lastActivitySet("comment");
        turn->toolSet(nullptr);
    }
}

void Model::chatReset(const QString &messageId) {
    auto *chat = m_chatIndex.value(messageId);
    chat->contentReset();
}

void Model::activitySet(const QString &turnId, const QString &activity, const QUrl &activityIcon) {
    if (m_turns.isEmpty()) return;
    turnGet(turnId)->activitySet(activity, activityIcon);
}

Subagent *Model::subagentCreate(const QString &turnId, const QString &runtimeId, const QString &role, const QString &activity) {
    auto *turn = turnGet(turnId);
    auto *subagent = new Subagent(runtimeId, role, activity, turn); // NOLINT
    turn->subagentAppend(subagent);
    return subagent;
}

Turn *Model::turnGet(const QString &turnId) const {
    return turnId.isEmpty() ? m_turns.constLast() : m_turnIndex.value(turnId);
}

}

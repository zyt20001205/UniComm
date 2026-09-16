#ifndef UNICOMM_CONVERSATIONMODEL_H
#define UNICOMM_CONVERSATIONMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QUrl>

namespace Conversation {

class Message;
class Subagent;
class Turn;

class Model final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        TurnRole = Qt::UserRole + 1
    };

    explicit Model(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void clear();

    void turnCreate(const QString &turnId, qint64 startedAt);

    void turnFinish(const QString &turnId, qint64 finishedAt);

    void chatCreate(const QString &turnId, const QString &messageId, const QString &role, const QList<QUrl> &attachments);

    void chatAppend(const QString &messageId, const QString &text);

    void chatReset(const QString &messageId);

    void activitySet(const QString &turnId, const QString &activity, const QUrl &activityIcon = {});

    [[nodiscard]] Subagent *subagentCreate(const QString &turnId, const QString &runtimeId, const QString &role, const QString &activity);

private:
    [[nodiscard]] Turn *turnGet(const QString &turnId) const;

    QList<Turn *> m_turns{};
    QHash<QString, Turn *> m_turnIndex{};
    QHash<QString, Message *> m_chatIndex{};
    QHash<QString, Turn *> m_chatTurns{};
};

}

#endif //UNICOMM_CONVERSATIONMODEL_H

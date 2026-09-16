#ifndef UNICOMM_TURN_H
#define UNICOMM_TURN_H

#include <QUrl>
#include <QVariantList>

namespace Conversation {
    class Message;
    class Subagent;
    class Tool;

    class Turn final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString id READ idGet CONSTANT)
        Q_PROPERTY(double startedAt READ startedAtGet CONSTANT)
        Q_PROPERTY(double finishedAt READ finishedAtGet NOTIFY changeFinishedAt)
        Q_PROPERTY(QString prompt READ promptGet NOTIFY changePrompt)
        Q_PROPERTY(QString response READ responseGet NOTIFY changeResponse)
        Q_PROPERTY(bool collapsed READ collapsedGet WRITE collapsedSet NOTIFY changeCollapsed)
        Q_PROPERTY(QVariantList blocks READ blocksGet NOTIFY changeBlocks)
        Q_PROPERTY(QVariantList subagents READ subagentsGet NOTIFY changeSubagents)
        Q_PROPERTY(QString activity READ activityGet NOTIFY changeActivity)
        Q_PROPERTY(QUrl activityIcon READ activityIconGet NOTIFY changeActivity)

    public:
        Turn(const QString &id, qint64 startedAt, QObject *parent = nullptr);

        [[nodiscard]] QString idGet() const;

        [[nodiscard]] double startedAtGet() const;

        [[nodiscard]] double finishedAtGet() const;

        [[nodiscard]] QString promptGet() const;

        [[nodiscard]] QString responseGet() const;

        [[nodiscard]] Message *promptChatGet() const;

        [[nodiscard]] bool collapsedGet() const;

        [[nodiscard]] QVariantList blocksGet() const;

        [[nodiscard]] QVariantList subagentsGet() const;

        [[nodiscard]] QString activityGet() const;

        [[nodiscard]] QUrl activityIconGet() const;

        void blockAppend(QObject *block);

        void subagentAppend(Subagent *subagent);

        void promptChatSet(Message *chat);

        void finishedAtSet(qint64 finishedAt);

        void collapsedSet(bool collapsed);

        void activitySet(const QString &activity, const QUrl &activityIcon = {});

        [[nodiscard]] QString lastActivityGet() const;

        void lastActivitySet(const QString &activity);

        [[nodiscard]] Tool *toolGet() const;

        void toolSet(Tool *tool);

        [[nodiscard]] Message *lastAssistantGet() const;

        void lastAssistantSet(Message *assistant);

    signals:
        void changeFinishedAt();

        void changePrompt();

        void changeResponse();

        void changeCollapsed();

        void changeBlocks();

        void changeSubagents();

        void changeActivity();

    private:
        QString m_id{};
        qint64 m_startedAt{};
        qint64 m_finishedAt{};
        bool m_collapsed{false};
        QVariantList m_blocks{};
        QVariantList m_subagents{};
        QString m_activity{};
        QUrl m_activityIcon{};
        QString m_lastActivity{};
        Tool *m_tool{};
        Message *m_promptChat{};
        Message *m_lastAssistant{};
    };
}

#endif //UNICOMM_TURN_H

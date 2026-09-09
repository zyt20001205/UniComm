#ifndef UNICOMM_SQLMODULE_H
#define UNICOMM_SQLMODULE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class QSqlQuery;

class SqlModule final : public QObject {
    Q_OBJECT

public:
    struct TurnStatus {
        enum {
            None,
            Running,
            Completed,
            Aborted,
            Error
        };
    };

    struct Conversation {
        QString id{};
        QString title{};
        int strategy{};
        int mode{};
        QString provider{};
        QString model{};
        QString summary{};
        QString compactedTurnId{};
        qint64 contextTokens{};
        qint64 createdAt{};
        qint64 updatedAt{};
    };

    struct Usage {
        qint64 promptTokens{};
        qint64 completionTokens{};
        qint64 cacheHitTokens{};
        qint64 reasoningTokens{};
    };

    struct Timing {
        qint64 createdAt{};
        qint64 startedAt{};
        qint64 firstOutputAt{};
        qint64 finishedAt{};
    };

    struct Message {
        QString id{};
        QString conversationId{};
        QString turnId{};
        qint64 sequence{};
        QString role{};
        QString content{};
        QList<QUrl> attachments{};
        QString reasoningContent{};
        QString toolCallId{};
        QJsonArray toolCalls{};
        bool approved{false};
        int strategy{};
        QString provider{};
        QString model{};
        int status{TurnStatus::None};
        QString error{};
        Timing timing{};
        Usage usage{};
    };

    struct SearchResult {
        QString conversationId{};
        QString conversationTitle{};
        QString turnId{};
        qint64 createdAt{};
        double rank{};
        QList<Message> messages{};
    };

    explicit SqlModule(const QJsonObject &sqlConfig, QObject *parent = nullptr);

    ~SqlModule() override;

    [[nodiscard]] QList<Conversation> conversationsGet() const;

    [[nodiscard]] QPair<Conversation, QList<Message>> conversationGet(const QString &id) const;

    [[nodiscard]] QList<Message> turnGet(const QString &id) const;

    [[nodiscard]] QList<SearchResult> conversationsSearch(const QString &text, int limit) const;

    void conversationInsert(const Conversation &conversation) const;

    void conversationRename(const QString &id, const QString &title) const;

    void conversationDelete(const QString &id) const;

    void conversationStrategySet(const QString &id, int strategy) const;

    void conversationModeSet(const QString &id, int mode) const;

    void conversationModelSet(const QString &id, const QString &provider, const QString &model) const;

    void conversationCompact(const QString &id, const QString &summary, const QString &compactedTurnId) const;

    void conversationAppend(const QString &conversationId, const QList<Message> &messages, qint64 contextTokens) const;

    void conversationRollback(const QString &conversationId, const QString &turnId) const;

private:
    [[nodiscard]] static Message messageBuild(const QSqlQuery &query);

    [[nodiscard]] bool initialize() const;

    QJsonObject m_config{};
    QString m_connectionName{};
};

#endif //UNICOMM_SQLMODULE_H

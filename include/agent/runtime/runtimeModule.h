#ifndef UNICOMM_RUNTIMEMODULE_H
#define UNICOMM_RUNTIMEMODULE_H

#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>

#include "agent/module/sqlModule.h"

class BaseAgent;
class BaseProvider;
class ContextModule;
class ProviderModule;
class ToolsModule;
struct ToolResult;

struct RuntimeServices {
    ContextModule *contextModule{};
    ProviderModule *providerModule{};
    SqlModule *sqlModule{};
    ToolsModule *toolsModule{};
};

class RuntimeModule final : public QObject {
    Q_OBJECT

public:
    struct AgentState {
        enum {
            Ready,
            Abort,
            Error,
            Complete,

            Listen,
            STT,
            Pre,
            Compact,
            Request,
            Think,
            Response,
            ToolCall,
            Permission,
            UserInput,
            ToolExec,
            Speak
        };
    };

    struct AgentMode {
        enum {
            Chat,
            Read,
            Write,
            FullAccess
        };
    };

    explicit RuntimeModule(BaseAgent *agent, const RuntimeServices &services, QObject *parent = nullptr);

    [[nodiscard]] QString idGet() const;

    [[nodiscard]] QString roleGet() const;

    [[nodiscard]] int stateGet() const;

    [[nodiscard]] QString turnIdGet() const;

    void abort();

    void pre(const QString &conversationId, const QString &text, const QList<QUrl> &attachments);

    void steer(const QString &text);

    void compact(const QString &conversationId);

    void request(const QString &provider, const QString &model, int mode, const QString &prompt, const QList<QUrl> &attachments = {});

    void permission(bool status);

    void userInput(const QString &answer);

signals:
    void changeState();

    void showError(const QString &message);

    void createTurn(const QString &turnId, qint64 startedAt);

    void finishTurn(const QString &turnId, qint64 finishedAt);

    void createChat(const QString &turnId, const QString &messageId, const QString &role);

    void appendChat(const QString &messageId, const QString &text);

    void resetChat(const QString &messageId);

    void retryRequest(int attempt, int limit);

    void updateUsage(qint64 totalTokens);

    void finishRun(const QString &result, bool success);

private:
    struct ToolCall {
        QString id{};
        QString name{};
        QString arguments{};
        qsizetype messageIndex{-1};
        bool approved{false};
    };

    struct TurnContext {
        QString id{};
        QString conversationId{};
        QString provider{};
        QString model{};
        QString compactedTurnId{};
        int mode{AgentMode::Chat};
        QList<SqlModule::Message> messages{};
        QString steering{};
        qint64 currentUsage{};
        int status{SqlModule::TurnStatus::Running};
        QString error{};
        bool questionsAllowed{true};
        qsizetype toolCallCount{};
        qsizetype toolPlanCount{};
        QList<ToolCall> toolCalls{};
        qsizetype toolIndex{};
        QString failedTool{};
        int failureCount{};
    };

    void stateSet(int state, const QVariant &payload = QVariant());

    bool retry(QNetworkReply::NetworkError error, const BaseProvider *provider, const QJsonObject &body, qsizetype messageIndex, int retryCount);

    void _request(const BaseProvider *provider, const QJsonObject &body, qsizetype messageIndex = -1, int retryCount = 0);

    qsizetype conversationAppend(const QString &role, const QString &toolCallId = {});

    void toolResultSet(const ToolResult &result);

    QString m_id{};
    BaseAgent *m_agent{};
    TurnContext m_turn{};
    int m_state{AgentState::Ready};
    QNetworkReply *m_reply{};
    ContextModule *m_contextModule{};
    ProviderModule *m_providerModule{};
    SqlModule *m_sqlModule{};
    ToolsModule *m_toolsModule{};
};

#endif //UNICOMM_RUNTIMEMODULE_H

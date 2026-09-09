#include "agent/runtime/runtimeModule.h"

#include <iterator>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QTimer>
#include <QUuid>

#include "globals.h"
#include "agent/agentModule.h"
#include "agent/module/contextModule.h"
#include "agent/module/toolsModule.h"
#include "agent/provider/baseProvider.h"
#include "agent/provider/providerModule.h"
#include "agent/role/baseAgent.h"

// public
RuntimeModule::RuntimeModule(BaseAgent *agent, const RuntimeServices &services, QObject *parent)
    : QObject(parent),
      m_id(QUuid::createUuid().toString(QUuid::WithoutBraces)),
      m_agent(agent),
      m_contextModule(services.contextModule),
      m_providerModule(services.providerModule),
      m_sqlModule(services.sqlModule),
      m_toolsModule(services.toolsModule) {
    m_agent->setParent(this);
}

QString RuntimeModule::idGet() const {
    return m_id;
}

QString RuntimeModule::roleGet() const {
    return m_agent->roleGet();
}

int RuntimeModule::stateGet() const {
    return m_state;
}

QString RuntimeModule::turnIdGet() const {
    return m_turn.id;
}

void RuntimeModule::abort() {
    stateSet(AgentState::Abort);
}

void RuntimeModule::pre(const QString &conversationId, const QString &text, const QList<QUrl> &attachments) {
    m_turn = {
        .conversationId = conversationId,
        .messages = {SqlModule::Message{.content = text, .attachments = attachments}}
    };
    stateSet(AgentState::Pre);
}

void RuntimeModule::steer(const QString &text) {
    if (!m_turn.steering.isEmpty()) m_turn.steering.append('\n');
    m_turn.steering.append(text);
}

void RuntimeModule::compact(const QString &conversationId) {
    m_turn.conversationId = conversationId;
    stateSet(AgentState::Compact);
}

void RuntimeModule::request(const QString &provider, const QString &model, const int mode, const QString &prompt, const QList<QUrl> &attachments) {
    m_turn = {
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .provider = provider,
        .model = model,
        .mode = mode
    };
    const auto messageIndex = conversationAppend("user");
    auto &message = m_turn.messages[messageIndex];
    message.content = prompt;
    message.attachments = attachments;
    message.provider = provider;
    message.model = model;
    message.status = SqlModule::TurnStatus::Running;
    message.timing.startedAt = message.timing.createdAt;
    stateSet(AgentState::Request);
}

void RuntimeModule::permission(const bool status) {
    auto &toolCall = m_turn.toolCalls[m_turn.toolIndex];
    const auto &message = m_turn.messages.at(toolCall.messageIndex);
    toolCall.approved = status;
    if (toolCall.name != "plan_update" && toolCall.name != "user_input_request") emit appendChat(message.id, status ? " ✓" : " ✗");
    stateSet(AgentState::ToolExec);
}

void RuntimeModule::userInput(const QString &answer) {
    const auto text = answer.trimmed();
    if (text.isEmpty()) {
        m_turn.questionsAllowed = false;
        toolResultSet({"The user chose not to answer and disabled further questions for this turn. Continue using your best judgment."});
    } else {
        toolResultSet({text});
    }
}

// private
void RuntimeModule::stateSet(const int state, const QVariant &payload) {
    m_state = state;
    emit changeState();
    switch (state) {
        case AgentState::Ready: break;
        case AgentState::Abort: {
            m_turn.status = SqlModule::TurnStatus::Aborted;
            if (m_reply != nullptr) {
                m_reply->abort();
                break;
            }
            stateSet(AgentState::Complete);
        }
        break;
        case AgentState::Error: {
            m_turn.status = SqlModule::TurnStatus::Error;
            m_turn.error = payload.toString();
            emit showError(m_turn.error);
            stateSet(AgentState::Complete);
        }
        break;
        case AgentState::Complete: {
            if (m_turn.id.isEmpty()) {
                m_turn = {};
                stateSet(AgentState::Ready);
                break;
            }

            if (m_turn.conversationId.isEmpty()) {
                const auto result = m_turn.status == SqlModule::TurnStatus::Completed
                                        ? m_turn.messages.last().content
                                        : m_turn.status == SqlModule::TurnStatus::Error
                                              ? m_turn.error
                                              : QString("Agent task aborted.");
                const auto success = m_turn.status == SqlModule::TurnStatus::Completed;
                m_turn = {};
                stateSet(AgentState::Ready);
                emit finishRun(result, success);
                break;
            }

            const auto finishedAt = QDateTime::currentMSecsSinceEpoch();
            auto &user = m_turn.messages.first();
            user.status = m_turn.status;
            user.error = m_turn.error;
            user.timing.finishedAt = finishedAt;
            for (auto &message: m_turn.messages) {
                if (message.timing.finishedAt != 0) continue;
                message.timing.finishedAt = finishedAt;
            }
            m_sqlModule->conversationAppend(m_turn.conversationId, m_turn.messages, m_turn.currentUsage);
            emit finishTurn(m_turn.id, finishedAt);
            m_turn = {};
            stateSet(AgentState::Ready);
        }
        break;
        case AgentState::Pre: {
            const auto conversationId = m_turn.conversationId;
            const auto input = m_turn.messages.first();
            const auto conversation = m_sqlModule->conversationGet(conversationId).first;
            if (conversation.provider.isEmpty() || conversation.model.isEmpty()) {
                stateSet(AgentState::Error, tr("Please select a model first."));
                break;
            }

            m_turn = {
                .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
                .conversationId = conversationId,
                .provider = conversation.provider,
                .model = conversation.model,
                .mode = conversation.mode
            };
            const auto messageIndex = conversationAppend("user");
            auto &message = m_turn.messages[messageIndex];
            message.content = input.content;
            message.attachments = input.attachments;
            message.strategy = conversation.strategy;
            message.provider = conversation.provider;
            message.model = conversation.model;
            message.status = SqlModule::TurnStatus::Running;
            message.timing.startedAt = message.timing.createdAt;
            emit createTurn(m_turn.id, message.timing.createdAt);
            emit createChat(m_turn.id, message.id, message.role);
            emit appendChat(message.id, message.content);

            const auto model = m_providerModule->providerGet(conversation.provider)->modelGet(conversation.model);
            stateSet(m_contextModule->compactRequired(conversation.contextTokens, model.contextWindow) ? AgentState::Compact : AgentState::Request);
        }
        break;
        case AgentState::Compact: {
            const auto [conversation, history] = m_sqlModule->conversationGet(m_turn.conversationId);
            const auto [turnId, messages] = m_contextModule->compactBuild(conversation, history);
            m_turn.compactedTurnId = turnId;
            auto *provider = m_providerModule->providerGet(conversation.provider);
            const auto body = provider->requestBuild(conversation.model, messages, {}, false);
            _request(provider, body);
        }
        break;
        case AgentState::Request: {
            if (!m_turn.steering.isEmpty()) {
                const auto messageIndex = conversationAppend("user");
                auto &message = m_turn.messages[messageIndex];
                message.content = m_turn.steering;
                m_turn.steering.clear();
                emit createChat(m_turn.id, message.id, message.role);
                emit appendChat(message.id, message.content);
            }

            QJsonArray context{};
            QString providerId{};
            QString modelId{};
            QString steering{};
            const auto toolPlanInterval = g_agent->toolPlanIntervalGet();
            if (toolPlanInterval > 0 && m_agent->toolContains("plan_update") && m_turn.toolPlanCount >= toolPlanInterval) {
                steering = "Tool activity has reached the configured planning interval. Review progress and call plan_update if the plan is missing or outdated.";
                m_turn.toolPlanCount %= toolPlanInterval;
            }
            if (m_turn.conversationId.isEmpty()) {
                providerId = m_turn.provider;
                modelId = m_turn.model;
                context = m_contextModule->contextBuild(m_agent->systemGet(), m_turn.mode, m_turn.messages, steering);
            } else {
                const auto [conversation, messages] = m_sqlModule->conversationGet(m_turn.conversationId);
                providerId = conversation.provider;
                modelId = conversation.model;
                context = m_contextModule->contextBuild(m_agent->systemGet(), conversation, messages, m_turn.messages, steering);
            }
            const auto tools = m_turn.mode == AgentMode::Chat ? QJsonArray{} : m_agent->toolsGet(*m_toolsModule);
            auto *provider = m_providerModule->providerGet(providerId);
            const auto body = provider->requestBuild(modelId, context, tools, true);
            _request(provider, body);
        }
        break;
        case AgentState::ToolCall: {
            auto &toolCall = m_turn.toolCalls[m_turn.toolIndex];
            auto &message = m_turn.messages[toolCall.messageIndex];
            message.timing.startedAt = QDateTime::currentMSecsSinceEpoch();
            const auto [approved, text] = m_toolsModule->toolCall(m_turn.mode, toolCall.name, toolCall.arguments);
            toolCall.approved = approved;
            if (toolCall.name == "plan_update") {
                stateSet(AgentState::ToolExec);
            } else if (toolCall.name == "user_input_request") {
                if (m_turn.questionsAllowed) {
                    auto input = QJsonDocument::fromJson(toolCall.arguments.toUtf8()).object();
                    auto options = input.value("options").toArray();
                    while (options.size() > 3) options.removeLast();
                    input["options"] = options;
                    stateSet(AgentState::UserInput, input.toVariantMap());
                } else {
                    toolResultSet({"Further questions are disabled for this turn. Continue using the available context and your best judgment."});
                }
            } else {
                emit createChat(m_turn.id, message.id, "tool");
                emit appendChat(message.id, text);
                // permission check
                if (!toolCall.approved) {
                    stateSet(AgentState::Permission, text);
                    break;
                }
                emit appendChat(message.id, " ✓");
                stateSet(AgentState::ToolExec);
            }
        }
        break;
        case AgentState::Permission: {
            if (m_turn.conversationId.isEmpty()) g_agent->subagentUpdate(m_id, "Waiting for approval...");
            g_agent->permissionRequest(m_id, payload.toString());
        }
        break;
        case AgentState::UserInput: {
            const auto request = payload.toMap();
            if (m_turn.conversationId.isEmpty()) g_agent->subagentUpdate(m_id, "Waiting for input...");
            g_agent->userInputRequest(m_id, request);
        }
        break;
        case AgentState::ToolExec: {
            const auto toolCall = m_turn.toolCalls.at(m_turn.toolIndex);
            if (!toolCall.approved) {
                toolResultSet({"User denied permission to execute this tool."});
                break;
            }

            if (m_turn.conversationId.isEmpty()) g_agent->subagentUpdate(m_id, m_toolsModule->toolTextGet(toolCall.name, toolCall.arguments));
            const auto turnId = m_turn.id;
            m_turn.messages[toolCall.messageIndex].timing.startedAt = QDateTime::currentMSecsSinceEpoch();
            auto future = m_toolsModule->toolExecute(m_id, m_agent->roleGet(), toolCall.name, toolCall.arguments);
            future.then(this, [this, turnId, toolCall](const ToolResult &result) {
                if (m_state != AgentState::ToolExec || m_turn.id != turnId) return;
                if (m_turn.toolCalls.at(m_turn.toolIndex).id != toolCall.id) return;
                if (toolCall.name == "plan_update") {
                    if (result.success) m_turn.toolPlanCount = 0;
                } else {
                    ++m_turn.toolCallCount;
                    ++m_turn.toolPlanCount;
                }
                toolResultSet(result);
            });
        }
        break;
        case AgentState::Speak: stateSet(AgentState::Ready);
            break;
        default: break;
    }
}

bool RuntimeModule::retry(const QNetworkReply::NetworkError error, const BaseProvider *provider, const QJsonObject &body, const qsizetype messageIndex, const int retryCount) {
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::UnknownNetworkError:
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::ProtocolFailure:
        case QNetworkReply::InternalServerError:
        case QNetworkReply::ServiceUnavailableError:
        case QNetworkReply::UnknownServerError: break;
        default: return false;
    }

    static constexpr int intervals[]{1000, 3000};
    constexpr int limit = std::size(intervals);
    if (retryCount >= limit) return false;
    const auto turnId = m_turn.id;
    emit retryRequest(retryCount + 1, limit);
    QTimer::singleShot(intervals[retryCount], this, [this, provider, body, messageIndex, retryCount, turnId] {
        if (m_turn.id != turnId || m_state == AgentState::Ready) return;
        _request(provider, body, messageIndex, retryCount + 1);
    });
    return true;
}

void RuntimeModule::_request(const BaseProvider *provider, const QJsonObject &body, qsizetype messageIndex, const int retryCount) {
    const auto startedAt = QDateTime::currentMSecsSinceEpoch();
    const auto stream = body.value("stream").toBool();
    if (stream) {
        if (messageIndex < 0) {
            messageIndex = conversationAppend("assistant");
            auto &message = m_turn.messages[messageIndex];
            message.provider = m_turn.provider;
            message.model = body.value("model").toString();
            message.timing.createdAt = startedAt;
            message.timing.startedAt = startedAt;
            emit createChat(m_turn.id, message.id, "assistant");
        } else {
            auto &message = m_turn.messages[messageIndex];
            message.content.clear();
            message.reasoningContent.clear();
            message.toolCalls = {};
            message.usage = {};
            message.timing.firstOutputAt = 0;
            message.timing.finishedAt = 0;
            m_turn.currentUsage = 0;
            emit resetChat(message.id);
            emit updateUsage(0);
        }
    }

    auto *reply = g_networkAccessManager->post(provider->requestGet(), QJsonDocument(body).toJson());
    m_reply = reply;

    if (!stream) {
        connect(reply, &QNetworkReply::finished, this, [this, reply, provider, body, messageIndex, retryCount] {
            if (m_reply == reply) m_reply = nullptr;
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                stateSet(AgentState::Complete);
            } else if (reply->error() == QNetworkReply::NoError) {
                const auto object = QJsonDocument::fromJson(reply->readAll()).object();
                const auto summary = object.value("choices").toArray().at(0).toObject().value("message").toObject().value("content").toString();
                if (summary.isEmpty()) {
                    stateSet(AgentState::Error, "Context compact failed.");
                } else {
                    m_sqlModule->conversationCompact(m_turn.conversationId, summary, m_turn.compactedTurnId);
                    m_turn.compactedTurnId.clear();
                    emit updateUsage(0);
                    stateSet(m_turn.id.isEmpty() ? AgentState::Ready : AgentState::Request);
                }
            } else if (!retry(reply->error(), provider, body, messageIndex, retryCount)) {
                const auto data = reply->readAll();
                const auto message = QJsonDocument::fromJson(data).object().value("error").toObject().value("message").toString();
                stateSet(AgentState::Error, message.isEmpty() ? reply->errorString() : message);
            }
            reply->deleteLater();
        });
        return;
    }

    const auto messageId = m_turn.messages.at(messageIndex).id;
    auto toolCalls = QSharedPointer<QMap<int, ToolCall> >::create();
    auto finishReason = QSharedPointer<QString>::create();

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, messageIndex, messageId, toolCalls, finishReason] {
        if (reply->error() != QNetworkReply::NoError) return;
        while (reply->canReadLine()) {
            auto line = reply->readLine().trimmed();
            if (line.isEmpty()) continue;
            if (!line.startsWith("data: ")) continue;

            line = line.mid(6);
            if (line == "[DONE]") continue;

            const auto doc = QJsonDocument::fromJson(line);
            if (doc.isNull() || !doc.isObject()) continue;

            const auto object = doc.object();
            const auto usage = object.value("usage").toObject();
            if (!usage.isEmpty()) {
                m_turn.currentUsage = usage.value("total_tokens").toInt();
                m_turn.messages[messageIndex].usage = {
                    .promptTokens = usage.value("prompt_tokens").toInt(),
                    .completionTokens = usage.value("completion_tokens").toInt(),
                    .cacheHitTokens = usage.value("prompt_cache_hit_tokens").toInt(),
                    .reasoningTokens = usage.value("completion_tokens_details").toObject().value("reasoning_tokens").toInt()
                };
                emit updateUsage(m_turn.currentUsage);
            }

            const auto choices = object.value("choices").toArray();
            if (choices.isEmpty()) continue;

            const auto choice = choices.at(0).toObject();
            const auto reason = choice.value("finish_reason").toString();
            if (!reason.isEmpty()) *finishReason = reason;
            const auto delta = choice.value("delta").toObject();
            const auto reasoning = delta.value("reasoning_content").toString();
            const auto content = delta.value("content").toString();
            const auto deltaToolCalls = delta.value("tool_calls").toArray();
            if ((!reasoning.isEmpty() || !content.isEmpty() || !deltaToolCalls.isEmpty()) && m_turn.messages[messageIndex].timing.firstOutputAt == 0) {
                m_turn.messages[messageIndex].timing.firstOutputAt = QDateTime::currentMSecsSinceEpoch();
            }
            if (!reasoning.isEmpty()) {
                auto &message = m_turn.messages[messageIndex];
                if (message.reasoningContent.isEmpty()) stateSet(AgentState::Think);
                message.reasoningContent.append(reasoning);
            }

            if (!content.isEmpty()) {
                auto &message = m_turn.messages[messageIndex];
                if (message.content.isEmpty()) stateSet(AgentState::Response);
                message.content.append(content);
                emit appendChat(messageId, content);
            }

            for (const auto &value: deltaToolCalls) {
                const auto object = value.toObject();
                if (!object.contains("index")) continue;
                const auto index = object.value("index").toInt();
                auto toolCall = toolCalls->value(index);
                if (object.contains("id")) toolCall.id = object.value("id").toString();
                const auto function = object.value("function").toObject();
                if (function.contains("name")) toolCall.name = function.value("name").toString();
                if (function.contains("arguments")) toolCall.arguments.append(function.value("arguments").toString());
                (*toolCalls)[index] = toolCall;
            }
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, provider, body, messageIndex, retryCount, messageId, toolCalls, finishReason] {
        if (m_reply == reply) m_reply = nullptr;
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            stateSet(AgentState::Complete);
        } else if (reply->error() == QNetworkReply::NoError) {
            if (finishReason->isEmpty()) {
                if (!retry(QNetworkReply::RemoteHostClosedError, provider, body, messageIndex, retryCount)) stateSet(AgentState::Error, "Model response ended without a finish reason.");
                reply->deleteLater();
                return;
            }
            m_turn.messages[messageIndex].timing.finishedAt = QDateTime::currentMSecsSinceEpoch();
            if (*finishReason == "tool_calls") {
                QJsonArray _toolCalls{};
                for (auto toolCall: toolCalls->values()) {
                    if (toolCall.name.isEmpty()) continue;

                    if (toolCall.id.isEmpty()) toolCall.id = "call_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

                    _toolCalls.append(QJsonObject{
                        {"id", toolCall.id},
                        {"type", "function"},
                        {
                            "function", QJsonObject{
                                {"name", toolCall.name},
                                {"arguments", toolCall.arguments}
                            }
                        }
                    });
                    toolCall.messageIndex = conversationAppend("tool", toolCall.id);
                    m_turn.toolCalls.append(toolCall);
                }
                m_turn.messages[messageIndex].toolCalls = _toolCalls;
                stateSet(AgentState::ToolCall);
            } else if (*finishReason == "stop") {
                if (m_turn.steering.isEmpty()) {
                    m_turn.status = SqlModule::TurnStatus::Completed;
                    stateSet(AgentState::Complete);
                } else {
                    stateSet(AgentState::Request);
                }
            } else if (*finishReason == "length") stateSet(AgentState::Error, "Model output reached the length limit.");
            else if (*finishReason == "content_filter") stateSet(AgentState::Error, "Model output was blocked by the content filter.");
            else stateSet(AgentState::Error, "Invalid model finish reason: " + *finishReason);
        } else if (!retry(reply->error(), provider, body, messageIndex, retryCount)) {
            const auto data = reply->readAll();
            const auto doc = QJsonDocument::fromJson(data);
            const auto message = doc.object().value("error").toObject().value("message").toString();
            stateSet(AgentState::Error, message.isEmpty() ? reply->errorString() : message);
        }
        reply->deleteLater();
    });
}

qsizetype RuntimeModule::conversationAppend(const QString &role, const QString &toolCallId) {
    const auto index = m_turn.messages.size();
    m_turn.messages.append(SqlModule::Message{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .conversationId = m_turn.conversationId,
        .turnId = m_turn.id,
        .role = role,
        .toolCallId = toolCallId,
        .timing = {.createdAt = QDateTime::currentMSecsSinceEpoch()}
    });
    return index;
}

void RuntimeModule::toolResultSet(const ToolResult &result) {
    const auto &toolCall = m_turn.toolCalls.at(m_turn.toolIndex);
    auto &message = m_turn.messages[toolCall.messageIndex];
    message.content = result.content;
    message.approved = toolCall.approved;
    message.timing.finishedAt = QDateTime::currentMSecsSinceEpoch();
    ++m_turn.toolIndex;

    if (result.success) {
        m_turn.failedTool.clear();
        m_turn.failureCount = 0;
    } else if (m_turn.failedTool == toolCall.name) {
        ++m_turn.failureCount;
    } else {
        m_turn.failedTool = toolCall.name;
        m_turn.failureCount = 1;
    }

    if (m_turn.failureCount >= g_agent->toolFailureLimitGet()) {
        stateSet(AgentState::Error, QString("Tool '%1' failed %2 consecutive times.").arg(m_turn.failedTool).arg(m_turn.failureCount));
        return;
    }
    if (m_turn.toolCallCount >= g_agent->toolCallLimitGet()) {
        stateSet(AgentState::Error, QString("Tool call limit reached: %1.").arg(m_turn.toolCallCount));
        return;
    }
    stateSet(m_turn.toolIndex < m_turn.toolCalls.size() ? AgentState::ToolCall : AgentState::Request);
}

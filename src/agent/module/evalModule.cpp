#include "agent/module/evalModule.h"

#include <QJsonObject>
#include <QStandardItem>

#include "agent/agentModule.h"
#include "util/uniCast.h"

// public
EvalModule::EvalModule(SqlModule *sqlModule, QObject *parent)
    : QObject(parent), m_sqlModule(sqlModule), m_model(new EvalModel(this)) {
}

EvalModel *EvalModule::modelGet() const {
    return m_model;
}

void EvalModule::update(const QString &conversationId) const {
    m_model->update(conversationId, m_sqlModule->conversationGet(conversationId).second);
}

QVariantHash EvalModel::summaryGet() const {
    return m_summary;
}

QHash<int, QByteArray> EvalModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[NodeIdRole] = "nodeId";
    roles[TurnIdRole] = "turnId";
    roles[MessageRole] = "messageRole";
    roles[ContentRole] = "content";
    roles[ReasoningContentRole] = "reasoningContent";
    roles[ArgumentsRole] = "arguments";
    roles[ApprovedRole] = "approved";
    roles[StatusRole] = "status";
    roles[ErrorRole] = "error";
    roles[DurationValueRole] = "durationValue";
    roles[ToolDurationValueRole] = "toolDurationValue";
    return roles;
}

void EvalModel::update(const QString &conversationId, const QList<SqlModule::Message> &messages) {
    clear();
    m_summary.clear();
    for (qsizetype start = 0; start < messages.size();) {
        auto end = start + 1;
        while (end < messages.size() && messages.at(end).turnId == messages.at(start).turnId) ++end;
        if (messages.at(start).strategy != AgentModule::AgentStrategy::Team) turnBuild(messages.sliced(start, end - start), rowCount() + 1);
        start = end;
    }

    const auto runCount = rowCount();
    const auto durationTotal = m_summary.value("durationTotal").toLongLong();
    const auto durationMaximum = m_summary.value("durationMaximum").toLongLong();
    const auto toolDuration = m_summary.value("toolDuration").toLongLong();
    const auto ttftTotal = m_summary.value("ttftTotal").toLongLong();
    const auto promptTokens = m_summary.value("promptTokens").toLongLong();
    const auto completionTokens = m_summary.value("completionTokens").toLongLong();
    m_summary["runCount"] = runCount;
    m_summary["completedCount"] = m_summary.value("completedCount").toInt();
    m_summary["abortedCount"] = m_summary.value("abortedCount").toInt();
    m_summary["errorCount"] = m_summary.value("errorCount").toInt();
    m_summary["durationTotal"] = durationTotal;
    m_summary["durationTotalText"] = uni_cast<QDuration>(durationTotal).value;
    m_summary["durationAverage"] = uni_cast<QDuration>(runCount == 0 ? 0 : durationTotal / runCount).value;
    m_summary["durationMaximum"] = uni_cast<QDuration>(durationMaximum).value;
    m_summary["ttftAverage"] = uni_cast<QDuration>(runCount == 0 ? 0 : ttftTotal / runCount).value;
    m_summary["ttftMaximumValue"] = m_summary.value("ttftMaximumValue").toLongLong();
    m_summary["toolDuration"] = toolDuration;
    m_summary["toolDurationText"] = uni_cast<QDuration>(toolDuration).value;
    m_summary["promptTokens"] = uni_cast<QCompactNumber>(promptTokens).value;
    m_summary["completionTokens"] = uni_cast<QCompactNumber>(completionTokens).value;
    m_summary["totalTokens"] = uni_cast<QCompactNumber>(promptTokens + completionTokens).value;
    m_summary["tokenMaximum"] = m_summary.value("tokenMaximum").toLongLong();
    m_summary["turnCategories"] = m_summary.value("turnCategories").toStringList();
    m_summary["promptMissValues"] = m_summary.value("promptMissValues").toList();
    m_summary["completionValues"] = m_summary.value("completionValues").toList();
    m_summary["cacheValues"] = m_summary.value("cacheValues").toList();
    m_summary["ttftValues"] = m_summary.value("ttftValues").toList();
    m_summary["modelCalls"] = m_summary.value("modelCalls").toInt();
    m_summary["toolCalls"] = m_summary.value("toolCalls").toInt();
    m_summary["provider"] = m_summary.value("provider").toString();
    m_summary["model"] = m_summary.value("model").toString();
    m_summary["conversationId"] = conversationId;
    m_summary.remove("ttftTotal");
    emit changeSummary();
}

// private
void EvalModel::turnBuild(const QList<SqlModule::Message> &messages, const qsizetype index) {
    const auto &first = messages.constFirst();
    auto timing = first.timing;
    SqlModule::Usage usage{};
    qint64 toolDuration{};
    int modelCalls{};
    int toolCalls{};
    QHash<QString, QPair<QString, QString> > toolCallMap{};
    for (const auto &message: messages) {
        if (message.role == "assistant") {
            ++modelCalls;
            if (modelCalls == 1) timing.firstOutputAt = message.timing.firstOutputAt;
            usage.promptTokens += message.usage.promptTokens;
            usage.completionTokens += message.usage.completionTokens;
            usage.cacheHitTokens += message.usage.cacheHitTokens;
            usage.reasoningTokens += message.usage.reasoningTokens;
        } else if (message.role == "tool") {
            ++toolCalls;
            toolDuration += message.timing.finishedAt - message.timing.startedAt;
        }
        for (const auto &value: message.toolCalls) {
            const auto toolCall = value.toObject();
            const auto function = toolCall.value("function").toObject();
            toolCallMap.insert(toolCall.value("id").toString(), {function.value("name").toString(), function.value("arguments").toString()});
        }
        timing.finishedAt = qMax(timing.finishedAt, message.timing.finishedAt);
    }

    const auto duration = timing.finishedAt - timing.startedAt;
    const auto ttft = timing.firstOutputAt - timing.startedAt;
    const auto tokens = usage.promptTokens + usage.completionTokens;

    auto *traceItem = new QStandardItem(tr("Turn #%1 · %2M · %3T").arg(index).arg(modelCalls).arg(toolCalls)); // NOLINT
    auto *durationItem = new QStandardItem(uni_cast<QDuration>(duration).value); // NOLINT
    auto *ttftItem = new QStandardItem(uni_cast<QDuration>(ttft).value); // NOLINT
    auto *tokensItem = new QStandardItem(uni_cast<QCompactNumber>(tokens).value); // NOLINT
    for (auto *item: {traceItem, durationItem, ttftItem, tokensItem}) item->setData(first.turnId, NodeIdRole);
    traceItem->setData(first.turnId, TurnIdRole);
    traceItem->setData("turn", MessageRole);
    traceItem->setData(first.status, StatusRole);
    traceItem->setData(first.error, ErrorRole);
    traceItem->setData(duration, DurationValueRole);
    traceItem->setData(toolDuration, ToolDurationValueRole);

    for (const auto &message: messages) {
        const auto toolCall = toolCallMap.value(message.toolCallId);
        auto title = tr("Tool");
        if (message.role == "user") {
            title = tr("User");
        } else if (message.role == "assistant") {
            title = tr("Assistant");
        } else {
            title = toolCall.first;
        }
        const auto messageDuration = message.role == "user" ? 0 : message.timing.finishedAt - message.timing.startedAt;
        const auto messageTtft = message.role == "assistant" ? message.timing.firstOutputAt - message.timing.startedAt : 0;
        const auto messageTokens = message.usage.promptTokens + message.usage.completionTokens;

        auto *messageTraceItem = new QStandardItem(title); // NOLINT
        auto *messageDurationItem = new QStandardItem(message.role == "user" ? QString{} : uni_cast<QDuration>(messageDuration).value); // NOLINT
        auto *messageTtftItem = new QStandardItem(message.role == "assistant" ? uni_cast<QDuration>(messageTtft).value : QString{}); // NOLINT
        auto *messageTokensItem = new QStandardItem(message.role == "assistant" ? uni_cast<QCompactNumber>(messageTokens).value : QString{}); // NOLINT
        if (message.role == "user") messageTraceItem->setData(QUrl("qrc:/icon/solo.svg"), Qt::DecorationRole);
        else if (message.role == "assistant") messageTraceItem->setData(QUrl("qrc:/icon/model.svg"), Qt::DecorationRole);
        else messageTraceItem->setData(QUrl("qrc:/icon/wrenchScrewdriver.svg"), Qt::DecorationRole);
        for (auto *item: {messageTraceItem, messageDurationItem, messageTtftItem, messageTokensItem}) item->setData(message.id, NodeIdRole);
        messageTraceItem->setData(message.turnId, TurnIdRole);
        messageTraceItem->setData(message.role, MessageRole);
        messageTraceItem->setData(message.content, ContentRole);
        messageTraceItem->setData(message.reasoningContent, ReasoningContentRole);
        messageTraceItem->setData(toolCall.second, ArgumentsRole);
        messageTraceItem->setData(message.approved, ApprovedRole);
        messageTraceItem->setData(messageDuration, DurationValueRole);
        messageTraceItem->setData(message.role == "tool" ? messageDuration : 0, ToolDurationValueRole);
        traceItem->appendRow({messageTraceItem, messageDurationItem, messageTtftItem, messageTokensItem});
    }

    const auto promptMiss = usage.promptTokens - usage.cacheHitTokens;
    m_summary["durationTotal"] = m_summary.value("durationTotal").toLongLong() + duration;
    m_summary["durationMaximum"] = qMax(m_summary.value("durationMaximum").toLongLong(), duration);
    m_summary["toolDuration"] = m_summary.value("toolDuration").toLongLong() + toolDuration;
    m_summary["promptTokens"] = m_summary.value("promptTokens").toLongLong() + usage.promptTokens;
    m_summary["completionTokens"] = m_summary.value("completionTokens").toLongLong() + usage.completionTokens;
    m_summary["tokenMaximum"] = qMax(m_summary.value("tokenMaximum").toLongLong(), tokens);
    m_summary["ttftMaximumValue"] = qMax(m_summary.value("ttftMaximumValue").toLongLong(), ttft);
    m_summary["modelCalls"] = m_summary.value("modelCalls").toInt() + modelCalls;
    m_summary["toolCalls"] = m_summary.value("toolCalls").toInt() + toolCalls;
    auto turnCategories = m_summary.value("turnCategories").toStringList();
    turnCategories.append(QString::number(index));
    m_summary["turnCategories"] = turnCategories;
    auto promptMissValues = m_summary.value("promptMissValues").toList();
    promptMissValues.append(promptMiss);
    m_summary["promptMissValues"] = promptMissValues;
    auto completionValues = m_summary.value("completionValues").toList();
    completionValues.append(usage.completionTokens);
    m_summary["completionValues"] = completionValues;
    auto cacheValues = m_summary.value("cacheValues").toList();
    cacheValues.append(usage.cacheHitTokens);
    m_summary["cacheValues"] = cacheValues;
    auto ttftValues = m_summary.value("ttftValues").toList();
    ttftValues.append(ttft);
    m_summary["ttftValues"] = ttftValues;
    m_summary["ttftTotal"] = m_summary.value("ttftTotal").toLongLong() + ttft;
    m_summary["provider"] = first.provider;
    m_summary["model"] = first.model;
    switch (first.status) {
        case SqlModule::TurnStatus::Completed: m_summary["completedCount"] = m_summary.value("completedCount").toInt() + 1;
            break;
        case SqlModule::TurnStatus::Aborted: m_summary["abortedCount"] = m_summary.value("abortedCount").toInt() + 1;
            break;
        case SqlModule::TurnStatus::Error: m_summary["errorCount"] = m_summary.value("errorCount").toInt() + 1;
            break;
        default: break;
    }

    appendRow({traceItem, durationItem, ttftItem, tokensItem});
}

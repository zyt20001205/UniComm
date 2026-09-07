#include "agent/module/sqlModule.h"
#include "globals.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

// public
SqlModule::SqlModule(const QJsonObject &sqlConfig, QObject *parent)
    : QObject(parent),
      m_config(sqlConfig),
      m_connectionName("agent_" + QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    const auto metadataPath = QDir(g_workspaceUrl.toLocalFile()).filePath(".unicomm");
    auto database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    database.setDatabaseName(QDir(metadataPath).filePath("agent.db"));
    if (!database.open()) return;
    if (!initialize()) {
        database.close();
        return;
    }
}

SqlModule::~SqlModule() {
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QList<SqlModule::Conversation> SqlModule::conversationsGet() const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return {};

    QSqlQuery query(database);
    if (!query.exec(R"(
        SELECT id, title, strategy, mode, provider, model, created_at, updated_at
        FROM conversations
        ORDER BY updated_at DESC
    )")) {
        qDebug() << "agent database conversation get failed:" << query.lastError().text();
        return {};
    }

    QList<Conversation> conversations{};
    while (query.next()) {
        conversations.append(Conversation{
            .id = query.value(0).toString(),
            .title = query.value(1).toString(),
            .strategy = query.value(2).toInt(),
            .mode = query.value(3).toInt(),
            .provider = query.value(4).toString(),
            .model = query.value(5).toString(),
            .createdAt = query.value(6).toLongLong(),
            .updatedAt = query.value(7).toLongLong()
        });
    }
    return conversations;
}

QPair<SqlModule::Conversation, QList<SqlModule::Message> > SqlModule::conversationGet(const QString &id) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return {};

    QSqlQuery query(database);
    query.prepare(R"(
        SELECT id, title, strategy, mode, provider, model, summary, compacted_turn_id, context_tokens, created_at, updated_at
        FROM conversations
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    if (!query.exec() || !query.next()) return {};

    const Conversation conversation{
        .id = query.value(0).toString(),
        .title = query.value(1).toString(),
        .strategy = query.value(2).toInt(),
        .mode = query.value(3).toInt(),
        .provider = query.value(4).toString(),
        .model = query.value(5).toString(),
        .summary = query.value(6).toString(),
        .compactedTurnId = query.value(7).toString(),
        .contextTokens = query.value(8).toLongLong(),
        .createdAt = query.value(9).toLongLong(),
        .updatedAt = query.value(10).toLongLong()
    };

    QSqlQuery messageQuery(database);
    messageQuery.prepare(R"(
        SELECT id, conversation_id, turn_id, sequence, role, content, reasoning_content, tool_call_id, tool_calls, approved,
               strategy, provider, model, status, error, created_at, started_at, first_output_at, finished_at,
               prompt_tokens, completion_tokens, cache_hit_tokens, reasoning_tokens
        FROM messages
        WHERE conversation_id = :conversationId
        ORDER BY sequence
    )");
    messageQuery.bindValue(":conversationId", id);
    if (!messageQuery.exec()) {
        qDebug() << "agent database conversation get failed:" << messageQuery.lastError().text();
        return {};
    }

    QList<Message> messages{};
    while (messageQuery.next()) messages.append(messageBuild(messageQuery));
    return {conversation, messages};
}

QList<SqlModule::Message> SqlModule::turnGet(const QString &id) const {
    const auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return {};

    QSqlQuery query(database);
    query.prepare(R"(
        SELECT id, conversation_id, turn_id, sequence, role, content, reasoning_content, tool_call_id, tool_calls, approved,
               strategy, provider, model, status, error, created_at, started_at, first_output_at, finished_at,
               prompt_tokens, completion_tokens, cache_hit_tokens, reasoning_tokens
        FROM messages
        WHERE turn_id = :id
        ORDER BY sequence
    )");
    query.bindValue(":id", id);
    if (!query.exec()) return {};

    QList<Message> messages{};
    while (query.next()) messages.append(messageBuild(query));
    return messages;
}

QList<SqlModule::SearchResult> SqlModule::conversationsSearch(const QString &text, const int limit) const {
    const auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return {};

    QSqlQuery query(database);
    query.prepare(R"(
        SELECT messages.conversation_id, conversations.title, messages.turn_id, messages.created_at, bm25(messages_fts)
        FROM messages_fts
        JOIN messages ON messages.rowid = messages_fts.rowid
        JOIN conversations ON conversations.id = messages.conversation_id
        WHERE messages_fts MATCH :text
          AND messages.turn_id IS NOT NULL
        ORDER BY bm25(messages_fts)
        LIMIT :limit
    )");
    query.bindValue(":text", text);
    query.bindValue(":limit", limit * 8);
    if (!query.exec()) {
        qDebug() << "agent database conversations search failed:" << query.lastError().text();
        return {};
    }

    QSqlQuery messageQuery(database);
    messageQuery.prepare(R"(
        SELECT id, conversation_id, turn_id, sequence, role, content, reasoning_content, tool_call_id, tool_calls, approved,
               strategy, provider, model, status, error, created_at, started_at, first_output_at, finished_at,
               prompt_tokens, completion_tokens, cache_hit_tokens, reasoning_tokens
        FROM messages
        WHERE conversation_id = :conversationId AND turn_id = :turnId
        ORDER BY sequence
    )");

    QSet<QString> turns{};
    QList<SearchResult> results{};
    while (query.next() && results.size() < limit) {
        const auto conversationId = query.value(0).toString();
        const auto turnId = query.value(2).toString();
        const auto key = conversationId + '\n' + turnId;
        if (turns.contains(key)) continue;
        turns.insert(key);

        messageQuery.bindValue(":conversationId", conversationId);
        messageQuery.bindValue(":turnId", turnId);
        if (!messageQuery.exec()) {
            qDebug() << "agent database search result get failed:" << messageQuery.lastError().text();
            return {};
        }

        QList<Message> messages{};
        while (messageQuery.next()) messages.append(messageBuild(messageQuery));
        results.append(SearchResult{
            .conversationId = conversationId,
            .conversationTitle = query.value(1).toString(),
            .turnId = turnId,
            .createdAt = messages.first().timing.createdAt,
            .rank = query.value(4).toDouble(),
            .messages = messages
        });
    }
    return results;
}

void SqlModule::conversationInsert(const Conversation &conversation) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO conversations (id, title, strategy, mode, provider, model, created_at, updated_at)
        VALUES (:id, :title, :strategy, :mode, :provider, :model, :createdAt, :updatedAt)
    )");
    query.bindValue(":id", conversation.id);
    query.bindValue(":title", conversation.title);
    query.bindValue(":strategy", conversation.strategy);
    query.bindValue(":mode", conversation.mode);
    query.bindValue(":provider", conversation.provider);
    query.bindValue(":model", conversation.model);
    query.bindValue(":createdAt", conversation.createdAt);
    query.bindValue(":updatedAt", conversation.updatedAt);
    if (query.exec()) return;
    qDebug() << "agent database conversation insert failed:" << query.lastError().text();
}

void SqlModule::conversationRename(const QString &id, const QString &title) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        UPDATE conversations
        SET title = :title, updated_at = :updatedAt
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    query.bindValue(":title", title);
    query.bindValue(":updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (query.exec()) return;
    qDebug() << "agent database conversation rename failed:" << query.lastError().text();
}

void SqlModule::conversationDelete(const QString &id) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare("DELETE FROM conversations WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec()) return;
    qDebug() << "agent database conversation delete failed:" << query.lastError().text();
}

void SqlModule::conversationStrategySet(const QString &id, const int strategy) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        UPDATE conversations
        SET strategy = :strategy, updated_at = :updatedAt
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    query.bindValue(":strategy", strategy);
    query.bindValue(":updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (query.exec()) return;
    qDebug() << "agent database conversation strategy set failed:" << query.lastError().text();
}

void SqlModule::conversationModeSet(const QString &id, const int mode) const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        UPDATE conversations
        SET mode = :mode, updated_at = :updatedAt
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    query.bindValue(":mode", mode);
    query.bindValue(":updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (query.exec()) return;
    qDebug() << "agent database conversation mode set failed:" << query.lastError().text();
}

void SqlModule::conversationModelSet(const QString &id, const QString &provider, const QString &model) const {
    const auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        UPDATE conversations
        SET provider = :provider, model = :model, updated_at = :updatedAt
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    query.bindValue(":provider", provider);
    query.bindValue(":model", model);
    query.bindValue(":updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (query.exec()) return;
    qDebug() << "agent database conversation model set failed:" << query.lastError().text();
}

void SqlModule::conversationCompact(const QString &id, const QString &summary, const QString &compactedTurnId) const {
    const auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        UPDATE conversations
        SET summary = :summary, compacted_turn_id = :compactedTurnId, context_tokens = 0, updated_at = :updatedAt
        WHERE id = :id
    )");
    query.bindValue(":id", id);
    query.bindValue(":summary", summary);
    query.bindValue(":compactedTurnId", compactedTurnId);
    query.bindValue(":updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (query.exec()) return;
    qDebug() << "agent database conversation compact failed:" << query.lastError().text();
}

void SqlModule::conversationAppend(const QString &conversationId, const QList<Message> &messages, const qint64 contextTokens) const {
    if (messages.isEmpty()) return;

    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen() || !database.transaction()) return;

    QSqlQuery sequenceQuery(database);
    sequenceQuery.prepare(R"(
        SELECT COALESCE(MAX(sequence) + 1, 0)
        FROM messages
        WHERE conversation_id = :conversationId
    )");
    sequenceQuery.bindValue(":conversationId", conversationId);
    if (!sequenceQuery.exec() || !sequenceQuery.next()) {
        qDebug() << "agent database message sequence get failed:" << sequenceQuery.lastError().text();
        database.rollback();
        return;
    }
    auto sequence = sequenceQuery.value(0).toLongLong();
    sequenceQuery.finish();

    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO messages (
            id, conversation_id, turn_id, sequence, role, content, reasoning_content, tool_call_id, tool_calls, approved,
            strategy, provider, model, status, error, created_at, started_at, first_output_at, finished_at,
            prompt_tokens, completion_tokens, cache_hit_tokens, reasoning_tokens
        )
        VALUES (
            :id, :conversationId, :turnId, :sequence, :role, :content, :reasoningContent, :toolCallId, :toolCalls, :approved,
            :strategy, :provider, :model, :status, :error, :createdAt, :startedAt, :firstOutputAt, :finishedAt,
            :promptTokens, :completionTokens, :cacheHitTokens, :reasoningTokens
        )
    )");
    for (const auto &message: messages) {
        query.bindValue(":id", message.id);
        query.bindValue(":conversationId", message.conversationId);
        query.bindValue(":turnId", message.turnId);
        query.bindValue(":sequence", sequence++);
        query.bindValue(":role", message.role);
        query.bindValue(":content", message.content);
        query.bindValue(":reasoningContent", message.reasoningContent);
        query.bindValue(":toolCallId", message.toolCallId);
        query.bindValue(":toolCalls", QString::fromUtf8(QJsonDocument(message.toolCalls).toJson(QJsonDocument::Compact)));
        query.bindValue(":approved", message.approved);
        query.bindValue(":strategy", message.strategy);
        query.bindValue(":provider", message.provider);
        query.bindValue(":model", message.model);
        query.bindValue(":status", message.status);
        query.bindValue(":error", message.error);
        query.bindValue(":createdAt", message.timing.createdAt);
        query.bindValue(":startedAt", message.timing.startedAt);
        query.bindValue(":firstOutputAt", message.timing.firstOutputAt);
        query.bindValue(":finishedAt", message.timing.finishedAt);
        query.bindValue(":promptTokens", message.usage.promptTokens);
        query.bindValue(":completionTokens", message.usage.completionTokens);
        query.bindValue(":cacheHitTokens", message.usage.cacheHitTokens);
        query.bindValue(":reasoningTokens", message.usage.reasoningTokens);
        if (query.exec()) continue;

        qDebug() << "agent database message insert failed:" << query.lastError().text();
        database.rollback();
        return;
    }

    query.prepare(R"(
        UPDATE conversations
        SET context_tokens = :contextTokens, updated_at = :updatedAt
        WHERE id = :conversationId
    )");
    query.bindValue(":conversationId", conversationId);
    query.bindValue(":contextTokens", contextTokens);
    query.bindValue(":updatedAt", messages.constLast().timing.finishedAt);
    if (!query.exec()) {
        qDebug() << "agent database conversation update failed:" << query.lastError().text();
        database.rollback();
        return;
    }

    if (!database.commit()) database.rollback();
}

void SqlModule::conversationRollback(const QString &conversationId, const QString &turnId) const {
    const auto [conversation, messages] = conversationGet(conversationId);
    const auto resetCompaction = conversation.compactedTurnId == turnId;
    qint64 contextTokens{};
    for (auto i = messages.size() - 1; i >= 0; --i) {
        const auto &message = messages.at(i);
        if (message.turnId == turnId) continue;
        if (message.turnId == conversation.compactedTurnId) break;
        const auto usage = message.usage.promptTokens + message.usage.completionTokens;
        if (usage == 0) continue;
        contextTokens = usage;
        break;
    }

    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen() || !database.transaction()) return;

    QSqlQuery query(database);
    query.prepare(R"(
        DELETE FROM messages
        WHERE conversation_id = :conversationId AND turn_id = :turnId
    )");
    query.bindValue(":conversationId", conversationId);
    query.bindValue(":turnId", turnId);
    if (!query.exec()) {
        qDebug() << "agent database turn delete failed:" << query.lastError().text();
        database.rollback();
        return;
    }

    query.prepare(R"(
        UPDATE conversations
        SET summary = :summary, compacted_turn_id = :compactedTurnId, context_tokens = :contextTokens
        WHERE id = :conversationId
    )");
    query.bindValue(":conversationId", conversationId);
    query.bindValue(":summary", resetCompaction ? QString{} : conversation.summary);
    query.bindValue(":compactedTurnId", resetCompaction ? QString{} : conversation.compactedTurnId);
    query.bindValue(":contextTokens", contextTokens);
    if (!query.exec()) {
        qDebug() << "agent database conversation rollback failed:" << query.lastError().text();
        database.rollback();
        return;
    }

    if (!database.commit()) database.rollback();
}

// private
SqlModule::Message SqlModule::messageBuild(const QSqlQuery &query) {
    const auto toolCalls = QJsonDocument::fromJson(query.value(8).toString().toUtf8());
    return {
        .id = query.value(0).toString(),
        .conversationId = query.value(1).toString(),
        .turnId = query.value(2).toString(),
        .sequence = query.value(3).toLongLong(),
        .role = query.value(4).toString(),
        .content = query.value(5).toString(),
        .reasoningContent = query.value(6).toString(),
        .toolCallId = query.value(7).toString(),
        .toolCalls = toolCalls.isArray() ? toolCalls.array() : QJsonArray{},
        .approved = query.value(9).toBool(),
        .strategy = query.value(10).toInt(),
        .provider = query.value(11).toString(),
        .model = query.value(12).toString(),
        .status = query.value(13).toInt(),
        .error = query.value(14).toString(),
        .timing = {
            .createdAt = query.value(15).toLongLong(),
            .startedAt = query.value(16).toLongLong(),
            .firstOutputAt = query.value(17).toLongLong(),
            .finishedAt = query.value(18).toLongLong()
        },
        .usage = {
            .promptTokens = query.value(19).toLongLong(),
            .completionTokens = query.value(20).toLongLong(),
            .cacheHitTokens = query.value(21).toLongLong(),
            .reasoningTokens = query.value(22).toLongLong()
        }
    };
}

bool SqlModule::initialize() const {
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.isOpen()) return false;

    const QStringList pragmas{
        "PRAGMA foreign_keys = ON",
        "PRAGMA journal_mode = WAL",
        "PRAGMA synchronous = NORMAL",
        "PRAGMA busy_timeout = 5000"
    };
    for (const auto &statement: pragmas) {
        QSqlQuery query(database);
        if (query.exec(statement)) continue;
        qDebug() << "agent database pragma failed:" << query.lastError().text();
        return false;
    }

    if (!database.transaction()) return false;
    const QStringList schema{
        R"(
            CREATE TABLE IF NOT EXISTS agent_meta (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS conversations (
                id TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                strategy INTEGER NOT NULL,
                mode INTEGER,
                provider TEXT,
                model TEXT,
                summary TEXT NOT NULL DEFAULT '',
                compacted_turn_id TEXT NOT NULL DEFAULT '',
                context_tokens INTEGER NOT NULL DEFAULT 0,
                created_at INTEGER NOT NULL,
                updated_at INTEGER NOT NULL
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS messages (
                id TEXT PRIMARY KEY,
                conversation_id TEXT NOT NULL,
                turn_id TEXT,
                sequence INTEGER NOT NULL,
                role TEXT NOT NULL,
                content TEXT,
                reasoning_content TEXT,
                tool_call_id TEXT,
                tool_calls TEXT,
                approved INTEGER NOT NULL DEFAULT 0,
                strategy INTEGER NOT NULL DEFAULT 0,
                provider TEXT,
                model TEXT,
                status INTEGER NOT NULL DEFAULT 0,
                error TEXT,
                created_at INTEGER NOT NULL,
                started_at INTEGER NOT NULL DEFAULT 0,
                first_output_at INTEGER NOT NULL DEFAULT 0,
                finished_at INTEGER NOT NULL DEFAULT 0,
                prompt_tokens INTEGER NOT NULL DEFAULT 0,
                completion_tokens INTEGER NOT NULL DEFAULT 0,
                cache_hit_tokens INTEGER NOT NULL DEFAULT 0,
                reasoning_tokens INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (conversation_id) REFERENCES conversations(id) ON DELETE CASCADE,
                UNIQUE (conversation_id, sequence)
            )
        )",
        R"(
            CREATE VIRTUAL TABLE IF NOT EXISTS messages_fts USING fts5(
                content,
                reasoning_content,
                content = 'messages',
                content_rowid = 'rowid'
            )
        )",
        R"(
            CREATE TRIGGER IF NOT EXISTS messages_fts_insert AFTER INSERT ON messages BEGIN
                INSERT INTO messages_fts(rowid, content, reasoning_content)
                VALUES (new.rowid, new.content, new.reasoning_content);
            END
        )",
        R"(
            CREATE TRIGGER IF NOT EXISTS messages_fts_delete AFTER DELETE ON messages BEGIN
                INSERT INTO messages_fts(messages_fts, rowid, content, reasoning_content)
                VALUES ('delete', old.rowid, old.content, old.reasoning_content);
            END
        )",
        R"(
            CREATE TRIGGER IF NOT EXISTS messages_fts_update AFTER UPDATE ON messages BEGIN
                INSERT INTO messages_fts(messages_fts, rowid, content, reasoning_content)
                VALUES ('delete', old.rowid, old.content, old.reasoning_content);
                INSERT INTO messages_fts(rowid, content, reasoning_content)
                VALUES (new.rowid, new.content, new.reasoning_content);
            END
        )",
        "CREATE INDEX IF NOT EXISTS conversations_updated_at ON conversations(updated_at DESC)",
        "CREATE INDEX IF NOT EXISTS messages_turn_id ON messages(turn_id, sequence)",
        "CREATE INDEX IF NOT EXISTS messages_conversation_id ON messages(conversation_id, sequence)",
        "PRAGMA user_version = 7"
    };
    for (const auto &statement: schema) {
        QSqlQuery query(database);
        if (query.exec(statement)) continue;
        qDebug() << "agent database schema failed:" << query.lastError().text();
        database.rollback();
        return false;
    }

    if (database.commit()) return true;
    qDebug() << "agent database commit failed:" << database.lastError().text();
    database.rollback();
    return false;
}

#include "agent/module/mcpModule.h"

#include <QFuture>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPromise>
#include <QSharedPointer>
#include <QStandardItem>
#include <QStringList>

#include <utility>

#include "globals.h"
#include "core/globalManager.h"
#include "agent/module/toolsModule.h"

// public
McpModule::McpModule(const QJsonObject &mcpConfig, QObject *parent)
    : QObject(parent),
      m_mcpModel(new McpModel(this)) {
    for (auto it = mcpConfig.begin(); it != mcpConfig.end(); ++it) {
        const auto url = QUrl(it.key());
        auto *item = new QStandardItem(url.host()); // NOLINT
        item->setData(url, McpModel::UrlRole);
        item->setData(it.value().toBool(), McpModel::EnabledRole);
        m_mcpModel->appendRow(item);
        m_servers.insert(url, Server{item});
    }
}

void McpModule::initialize() {
    for (auto it = m_servers.cbegin(); it != m_servers.cend(); ++it) {
        if (!it.value().item->data(McpModel::EnabledRole).toBool()) continue;
        serverDiscover(it.key());
    }
}

QString McpModule::serverInsert(const QUrl &serverUrl) {
    if (!serverUrl.isValid() || serverUrl.host().isEmpty() || (serverUrl.scheme() != "http" && serverUrl.scheme() != "https")) {
        return tr("Enter a valid HTTP(S) URL.");
    }
    if (m_servers.contains(serverUrl)) return tr("This MCP server already exists.");

    auto *item = new QStandardItem(serverUrl.host()); // NOLINT
    item->setData(serverUrl, McpModel::UrlRole);
    item->setData(true, McpModel::EnabledRole);
    m_mcpModel->appendRow(item);
    m_servers.insert(serverUrl, Server{item});
    serverDiscover(serverUrl);
    return {};
}

void McpModule::serverRemove(const QUrl &serverUrl) {
    const auto row = m_servers.value(serverUrl).item->row();
    m_servers.remove(serverUrl);
    m_mcpModel->removeRow(row);
    toolsRemove(serverUrl);
    toolsRegister();
}

void McpModule::enabledSet(const QUrl &serverUrl, const bool enabled) {
    m_servers.value(serverUrl).item->setData(enabled, McpModel::EnabledRole);
    if (enabled) serverDiscover(serverUrl);
    else {
        toolsRemove(serverUrl);
        toolsRegister();
    }
}

QFuture<ToolResult> McpModule::toolExecute(const QString &name, const QString &arguments) {
    if (!m_tools.contains(name)) return QtFuture::makeReadyValueFuture(ToolResult{"Unknown MCP tool.", false});
    const auto tool = m_tools.value(name);
    const auto params = QJsonObject{
        {"name", tool.name},
        {"arguments", QJsonDocument::fromJson(arguments.toUtf8()).object()}
    };
    return request(tool.serverUrl, "tools/call", params, tool.name).then(this, [](const QJsonObject &response) {
        if (response.contains("error")) {
            return ToolResult{QString::fromUtf8(QJsonDocument(response.value("error").toObject()).toJson(QJsonDocument::Compact)), false};
        }

        const auto result = response.value("result").toObject();
        QStringList text{};
        for (const auto &value: result.value("content").toArray()) {
            const auto content = value.toObject();
            if (content.value("type") == "text") text.append(content.value("text").toString());
        }
        const auto success = !result.value("isError").toBool();
        if (text.isEmpty()) return ToolResult{QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)), success};

        const auto output = text.join('\n');
        return ToolResult{success ? output : "MCP tool error: " + output, success};
    });
}

bool McpModule::toolContains(const QString &name) const {
    return m_tools.contains(name);
}

bool McpModule::toolReadOnly(const QString &name) const {
    return m_tools.value(name).readOnly;
}

McpModel *McpModule::mcpModelGet() const {
    return m_mcpModel;
}

// private
void McpModule::serverDiscover(const QUrl &serverUrl) {
    auto &server = m_servers[serverUrl];
    server.protocolVersion = StatelessVersion;
    server.sessionId.clear();

    request(serverUrl, "server/discover", {}).then(this, [this, serverUrl](const QJsonObject &response) {
        if (!m_servers.contains(serverUrl)) return;
        if (!response.contains("error")) {
            serverUpdate(serverUrl, response.value("result").toObject());
            return;
        }

        const auto error = response.value("error").toObject();
        const auto message = error.value("message").toString();
        if (error.value("code").toInt() != -32601 && !message.contains("Unsupported protocol version", Qt::CaseInsensitive)) return;
        serverInitialize(serverUrl);
    });
}

void McpModule::serverInitialize(const QUrl &serverUrl) {
    auto &server = m_servers[serverUrl];
    server.protocolVersion = StatefulVersion;
    server.sessionId.clear();
    const auto params = QJsonObject{
        {"protocolVersion", StatefulVersion},
        {"capabilities", QJsonObject{}},
        {
            "clientInfo", QJsonObject{
                {"name", "UniComm"},
                {"version", QCoreApplication::applicationVersion()}
            }
        }
    };
    request(serverUrl, "initialize", params).then(this, [this, serverUrl](const QJsonObject &response) {
        if (response.contains("error") || !m_servers.contains(serverUrl)) return;
        const auto result = response.value("result").toObject();
        m_servers[serverUrl].protocolVersion = result.value("protocolVersion").toString();
        serverNotify(serverUrl, result);
    });
}

void McpModule::serverNotify(const QUrl &serverUrl, const QJsonObject &result) {
    request(serverUrl, "notifications/initialized", {}).then(this, [this, serverUrl, result](const QJsonObject &response) {
        if (response.contains("error")) return;
        serverUpdate(serverUrl, result);
    });
}

void McpModule::serverUpdate(const QUrl &serverUrl, const QJsonObject &result) {
    if (!m_servers.contains(serverUrl)) return;
    auto serverInfo = result.value("serverInfo").toObject();
    if (serverInfo.isEmpty()) serverInfo = result.value("_meta").toObject().value("io.modelcontextprotocol/serverInfo").toObject();
    auto name = serverInfo.value("title").toString();
    if (name.isEmpty()) name = serverInfo.value("name").toString();
    auto *item = m_servers.value(serverUrl).item;
    if (!name.isEmpty()) item->setText(name);
    item->setData(serverInfo.value("version").toString(), McpModel::VersionRole);
    item->setData(serverInfo.value("description").toString(), McpModel::DescriptionRole);
    item->setData(serverInfo.value("websiteUrl").toString(), McpModel::WebsiteUrlRole);
    const auto icons = serverInfo.value("icons").toArray();
    item->setData(icons.isEmpty() ? QUrl{} : QUrl(icons.first().toObject().value("src").toString()), Qt::DecorationRole);
    item->setData(result.value("instructions").toString(), McpModel::InstructionsRole);
    QStringList supportedVersions{};
    for (const auto &value: result.value("supportedVersions").toArray()) supportedVersions.append(value.toString());
    if (supportedVersions.isEmpty() && result.contains("protocolVersion")) supportedVersions.append(result.value("protocolVersion").toString());
    item->setData(supportedVersions, McpModel::SupportedVersionsRole);
    item->setData(result.value("capabilities").toObject().toVariantMap(), McpModel::CapabilitiesRole);
    item->setData(result.value("cacheScope").toString(), McpModel::CacheScopeRole);
    item->setData(result.value("ttlMs").toInteger(), McpModel::TtlMsRole);
    toolsRemove(serverUrl);
    toolsRegister();
    toolsGet(serverUrl, {});
}

void McpModule::toolsGet(const QUrl &serverUrl, const QString &cursor) {
    QJsonObject params{};
    if (!cursor.isEmpty()) params["cursor"] = cursor;
    request(serverUrl, "tools/list", params).then(this, [this, serverUrl](const QJsonObject &response) {
        if (response.contains("error") || !m_servers.contains(serverUrl) || !m_servers.value(serverUrl).item->data(McpModel::EnabledRole).toBool()) return;

        const auto result = response.value("result").toObject();
        auto prefix = m_servers.value(serverUrl).item->text().toLower();
        if (prefix.isEmpty()) prefix = serverUrl.host().toLower();
        for (qsizetype index = 0; index < prefix.size(); ++index) {
            if (!prefix.at(index).isLetterOrNumber() && prefix.at(index) != '_' && prefix.at(index) != '-') prefix[index] = '_';
        }
        for (const auto &value: result.value("tools").toArray()) {
            const auto tool = value.toObject();
            const auto name = tool.value("name").toString();
            const auto exposedName = prefix + "__" + name;
            const QJsonObject definition{
                {"type", "function"},
                {
                    "function", QJsonObject{
                        {"name", exposedName},
                        {"description", tool.value("description")},
                        {"parameters", tool.value("inputSchema")}
                    }
                }
            };
            m_tools.insert(exposedName, Tool{
                serverUrl,
                name,
                definition,
                tool.value("annotations").toObject().value("readOnlyHint").toBool()
            });
        }

        const auto cursor = result.value("nextCursor").toString();
        if (cursor.isEmpty()) toolsRegister();
        else toolsGet(serverUrl, cursor);
    });
}

void McpModule::toolsRemove(const QUrl &serverUrl) {
    for (auto it = m_tools.begin(); it != m_tools.end();) {
        if (it.value().serverUrl == serverUrl) it = m_tools.erase(it);
        else ++it;
    }
}

void McpModule::toolsRegister() {
    QJsonArray tools{};
    for (const auto &tool: std::as_const(m_tools)) tools.append(tool.definition);
    emit registerTools(tools);
}

QFuture<QJsonObject> McpModule::request(const QUrl &serverUrl, const QString &method, QJsonObject params, const QString &name) {
    const auto server = m_servers.value(serverUrl);
    const auto stateless = server.protocolVersion == StatelessVersion;
    if (stateless) {
        params["_meta"] = QJsonObject{
            {"io.modelcontextprotocol/protocolVersion", server.protocolVersion},
            {
                "io.modelcontextprotocol/clientInfo", QJsonObject{
                    {"name", "UniComm"},
                    {"version", QCoreApplication::applicationVersion()}
                }
            },
            {"io.modelcontextprotocol/clientCapabilities", QJsonObject{}}
        };
    }

    const auto notification = method.startsWith("notifications/");
    const auto id = notification ? -1 : m_id++;
    QJsonObject body{
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    if (!notification) body["id"] = id;
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json, text/event-stream");
    request.setRawHeader("MCP-Protocol-Version", server.protocolVersion.toUtf8());
    if (stateless) {
        request.setRawHeader("Mcp-Method", method.toUtf8());
        if (!name.isEmpty()) request.setRawHeader("Mcp-Name", headerValueGet(name));
    } else if (!server.sessionId.isEmpty() && method != "initialize") {
        request.setRawHeader("Mcp-Session-Id", server.sessionId);
    }

    auto promise = QSharedPointer<QPromise<QJsonObject> >::create();
    promise->start();
    const auto future = promise->future();
    auto response = QSharedPointer<Response>::create();
    auto *reply = g_globalManager->networkAccessManagerGet()->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QIODevice::readyRead, this, [reply, response, id] {
        response->eventStream = reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("text/event-stream");
        response->buffer.append(reply->readAll());
        responseRead(*response, id, false);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, promise, response, serverUrl, notification, id] {
        response->eventStream = reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("text/event-stream");
        if (reply->isOpen()) response->buffer.append(reply->readAll());
        responseRead(*response, id, true);
        const auto sessionId = reply->rawHeader("Mcp-Session-Id");
        if (!sessionId.isEmpty() && m_servers.contains(serverUrl)) m_servers[serverUrl].sessionId = sessionId;
        if (response->object.isEmpty()) {
            if (notification && reply->error() == QNetworkReply::NoError) response->object["result"] = QJsonObject{};
            else response->object["error"] = QJsonObject{{"message", reply->error() == QNetworkReply::NoError ? "Invalid MCP response." : reply->errorString()}};
        }
        promise->addResult(response->object);
        promise->finish();
        reply->deleteLater();
    });
    return future;
}

QByteArray McpModule::headerValueGet(const QString &value) {
    const auto utf8 = value.toUtf8();
    auto safe = !utf8.isEmpty() && utf8.front() != ' ' && utf8.front() != '\t' && utf8.back() != ' ' && utf8.back() != '\t';
    for (const auto byte: utf8) safe = safe && byte >= 0x20 && byte <= 0x7e;
    if (utf8.startsWith("=?base64?") && utf8.endsWith("?=")) safe = false;
    return safe ? utf8 : "=?base64?" + utf8.toBase64() + "?=";
}

void McpModule::responseRead(Response &response, const int id, const bool finished) {
    if (!response.eventStream) {
        if (finished) response.object = QJsonDocument::fromJson(response.buffer).object();
        return;
    }

    if (finished && !response.buffer.endsWith("\n\n") && !response.buffer.endsWith("\r\n\r\n")) response.buffer.append("\n\n");
    while (true) {
        auto separator = response.buffer.indexOf("\r\n\r\n");
        auto separatorSize = 4;
        const auto lineFeedSeparator = response.buffer.indexOf("\n\n");
        if (separator < 0 || (lineFeedSeparator >= 0 && lineFeedSeparator < separator)) {
            separator = lineFeedSeparator;
            separatorSize = 2;
        }
        if (separator < 0) break;

        auto event = response.buffer.left(separator);
        response.buffer.remove(0, separator + separatorSize);
        event.replace("\r\n", "\n");
        QByteArray data{};
        for (const auto &line: event.split('\n')) {
            if (!line.startsWith("data:")) continue;
            auto value = line.mid(5);
            if (value.startsWith(' ')) value.remove(0, 1);
            if (!data.isEmpty()) data.append('\n');
            data.append(value);
        }
        const auto object = QJsonDocument::fromJson(data).object();
        if (object.value("id").toInt(-1) == id) response.object = object;
    }
}

QHash<int, QByteArray> McpModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[UrlRole] = "url";
    roles[EnabledRole] = "enabled";
    roles[VersionRole] = "version";
    roles[DescriptionRole] = "description";
    roles[WebsiteUrlRole] = "websiteUrl";
    roles[InstructionsRole] = "instructions";
    roles[SupportedVersionsRole] = "supportedVersions";
    roles[CapabilitiesRole] = "capabilities";
    roles[CacheScopeRole] = "cacheScope";
    roles[TtlMsRole] = "ttlMs";
    return roles;
}

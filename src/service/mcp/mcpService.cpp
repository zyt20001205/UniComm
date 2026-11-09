#include "service/mcp/mcpService.h"

#include <QDebug>
#include <QHttpHeaders>
#include <QHttpServerConfiguration>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QTcpServer>

#include "globals.h"
#include "agent/agentModule.h"
#include "agent/module/toolsModule.h"

McpService::McpService(QObject *parent)
    : QObject(parent) {
    QHttpServerConfiguration configuration{};
    configuration.setMaximumBodySize(64 * 1024);
    m_httpServer.setConfiguration(configuration);
    if (!m_httpServer.route("/mcp", this, [this](const QHttpServerRequest &request) {
        return requestHandle(request);
    })) {
        qDebug() << "MCP server: Could not register the MCP endpoint.";
        return;
    }

    constexpr quint16 port = 32167;
    auto *listener = new QTcpServer(&m_httpServer);
    if (!listener->listen(QHostAddress::LocalHost, port)) {
        qDebug("MCP server: %s", qPrintable(listener->errorString()));
        return;
    }
    if (!m_httpServer.bind(listener)) {
        listener->close();
        qDebug() << "MCP server: Could not bind the MCP endpoint.";
        return;
    }
    qDebug().noquote() << QString("MCP server listening on http://127.0.0.1:%1/mcp").arg(port);
}

QHttpServerResponse McpService::requestHandle(const QHttpServerRequest &request) const {
    using Status = QHttpServerResponse::StatusCode;
    constexpr auto ModernVersion = "2026-07-28";
    constexpr auto LegacyVersion = "2025-11-25";
    const QSet<QString> names{"port_list"};

    const auto& headers = request.headers();
    const auto port = QByteArray::number(request.localPort());
    const auto host = headers.value("Host").toByteArray().toLower();
    if (headers.contains("Origin") || (host != "127.0.0.1:" + port && host != "localhost:" + port))
        return {Status::Forbidden};
    if (request.method() != QHttpServerRequest::Method::Post) {
        QHttpHeaders responseHeaders{};
        responseHeaders.append("Allow", "POST");
        QHttpServerResponse response(Status::MethodNotAllowed);
        response.setHeaders(responseHeaders);
        return response;
    }
    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return QHttpServerResponse(QJsonObject{
            {"jsonrpc", "2.0"}, {"id", QJsonValue{}},
            {"error", QJsonObject{{"code", -32700}, {"message", "Parse error."}}}
        }, Status::BadRequest);

    const auto message = document.object();
    const auto id = message.value("id");
    const auto method = message.value("method").toString();
    if (message.value("jsonrpc") != "2.0" || method.isEmpty())
        return QHttpServerResponse(QJsonObject{
            {"jsonrpc", "2.0"}, {"id", QJsonValue{}},
            {"error", QJsonObject{{"code", -32600}, {"message", "Invalid request."}}}
        }, Status::BadRequest);
    if (id.isUndefined()) return {Status::Accepted};
    if ((method == "tools/list" || method == "tools/call") && (g_terminating || !g_agent))
        return QHttpServerResponse(QJsonObject{
            {"jsonrpc", "2.0"}, {"id", id},
            {"error", QJsonObject{{"code", -32603}, {"message", "Agent tools are unavailable."}}}
        }, Status::ServiceUnavailable);

    const auto params = message.value("params").toObject();
    const auto modern = headers.value("MCP-Protocol-Version") == ModernVersion || method == "server/discover";
    const auto serverInfo = QJsonObject{{"name", "UniComm"}, {"version", QCoreApplication::applicationVersion()}};
    const auto capabilities = QJsonObject{{"tools", QJsonObject{}}};
    QJsonObject result{};
    if (method == "initialize") {
        result = {
            {"protocolVersion", LegacyVersion},
            {"serverInfo", serverInfo},
            {"capabilities", capabilities}
        };
    } else if (method == "server/discover") {
        result = {
            {"supportedVersions", QJsonArray{ModernVersion}},
            {"capabilities", capabilities},
            {"_meta", QJsonObject{{"io.modelcontextprotocol/serverInfo", serverInfo}}}
        };
    } else if (method == "tools/list") {
        result = {{"tools", g_agent->runtimeServicesGet().toolsModule->schemasGet(names)}};
    } else if (method == "tools/call") {
        const auto name = params.value("name").toString();
        if (!names.contains(name))
            return QHttpServerResponse(QJsonObject{
                {"jsonrpc", "2.0"}, {"id", id},
                {"error", QJsonObject{{"code", -32602}, {"message", "Unknown tool."}}}
            });
        const auto tool = g_agent->runtimeServicesGet().toolsModule->toolExecute(name, params.value("arguments").toObject());
        result = {
            {"content", QJsonArray{QJsonObject{{"type", "text"}, {"text", tool.content}}}},
            {"isError", !tool.success}
        };
    } else if (method != "ping") {
        return QHttpServerResponse(QJsonObject{
            {"jsonrpc", "2.0"}, {"id", id},
            {"error", QJsonObject{{"code", -32601}, {"message", "Method not found."}}}
        }, modern ? Status::NotFound : Status::Ok);
    }
    if (modern) result["resultType"] = "complete";
    return QHttpServerResponse(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}});
}

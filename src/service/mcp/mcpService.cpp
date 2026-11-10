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

// public
McpService::McpService(QObject *parent)
    : BaseService("MCP", parent) {
    QHttpServerConfiguration configuration{};
    configuration.setMaximumBodySize(64 * 1024);
    m_httpServer.setConfiguration(configuration);
    if (!m_httpServer.route("/mcp", this, [this](const QHttpServerRequest &request) {
        return requestHandle(request);
    })) {
        const auto error = tr("Could not register the MCP endpoint.");
        detailSet(error.toHtmlEscaped());
        qDebug().noquote() << "MCP server:" << error;
        return;
    }

    constexpr quint16 port = 32167;
    auto *listener = new QTcpServer(&m_httpServer);
    if (!listener->listen(QHostAddress::LocalHost, port)) {
        const auto error = listener->errorString();
        detailSet(error.toHtmlEscaped());
        qDebug("MCP server: %s", qPrintable(error));
        return;
    }
    if (!m_httpServer.bind(listener)) {
        listener->close();
        const auto error = tr("Could not bind the MCP endpoint.");
        detailSet(error.toHtmlEscaped());
        qDebug().noquote() << "MCP server:" << error;
        return;
    }
    const auto endpoint = QString("http://127.0.0.1:%1/mcp").arg(port);
    detailSet(QString("<b>%1</b><br>%2").arg(tr("Endpoint"), endpoint));
}

// private
QFuture<QHttpServerResponse> McpService::requestHandle(const QHttpServerRequest &request) {
    constexpr auto ModernVersion = "2026-07-28";
    constexpr auto LegacyVersion = "2025-11-25";
    const QSet<QString> names{
        "api_list", "api_get", "demo_get",
        "port_list", "port_config_get", "port_create", "port_delete",
        "diagnostics_get", "script_exec"
    };
    const auto &headers = request.headers();
    const auto port = QByteArray::number(request.localPort());
    const auto host = headers.value("Host").toByteArray().toLower();
    if (headers.contains("Origin") || (host != "127.0.0.1:" + port && host != "localhost:" + port))
        return QtFuture::makeReadyValueFuture(QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden));
    if (request.method() != QHttpServerRequest::Method::Post) {
        QHttpHeaders responseHeaders{};
        responseHeaders.append("Allow", "POST");
        QHttpServerResponse response(QHttpServerResponse::StatusCode::MethodNotAllowed);
        response.setHeaders(responseHeaders);
        return QtFuture::makeReadyValueFuture(std::move(response));
    }
    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return QtFuture::makeReadyValueFuture(responseError({}, -32700, "Parse error.", QHttpServerResponse::StatusCode::BadRequest));

    const auto message = document.object();
    const auto id = message.value("id");
    const auto method = message.value("method").toString();
    if (message.value("jsonrpc") != "2.0" || method.isEmpty())
        return QtFuture::makeReadyValueFuture(responseError({}, -32600, "Invalid request.", QHttpServerResponse::StatusCode::BadRequest));
    if (id.isUndefined()) return QtFuture::makeReadyValueFuture(QHttpServerResponse(QHttpServerResponse::StatusCode::Accepted));
    if ((method == "tools/list" || method == "tools/call") && (g_terminating || !g_agent))
        return QtFuture::makeReadyValueFuture(responseError(id, -32603, "Agent tools are unavailable.", QHttpServerResponse::StatusCode::ServiceUnavailable));

    const auto params = message.value("params").toObject();
    const auto modern = headers.value("MCP-Protocol-Version") == ModernVersion || method == "server/discover";
    const auto serverInfo = QJsonObject{{"name", "UniComm"}, {"version", QCoreApplication::applicationVersion()}};
    const auto capabilities = QJsonObject{{"tools", QJsonObject{}}};
    QString event{method};
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
            return QtFuture::makeReadyValueFuture(responseError(id, -32602, "Unknown tool."));
        event += " · " + name;
        const auto arguments = QJsonDocument(params.value("arguments").toObject()).toJson(QJsonDocument::Compact);
        auto *tools = g_agent->runtimeServicesGet().toolsModule;
        return tools->toolExecute(name, QString::fromUtf8(arguments)).then(this, [this, id, event, modern](const ToolResult &tool) {
            return responseResult(id, event, {
                                      {"content", QJsonArray{QJsonObject{{"type", "text"}, {"text", tool.content}}}},
                                      {"isError", !tool.success}
                                  }, modern);
        });
    } else if (method != "ping") {
        return QtFuture::makeReadyValueFuture(responseError(
            id, -32601, "Method not found.", modern ? QHttpServerResponse::StatusCode::NotFound : QHttpServerResponse::StatusCode::Ok
        ));
    }
    return QtFuture::makeReadyValueFuture(responseResult(id, event, result, modern));
}

QHttpServerResponse McpService::responseResult(const QJsonValue &id, const QString &event, QJsonObject result, const bool modern) const {
    if (modern) result["resultType"] = "complete";
    const QJsonObject response{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
    eventAppend(event, response);
    return {response};
}

QHttpServerResponse McpService::responseError(const QJsonValue &id, const int code, const QString &message, const QHttpServerResponse::StatusCode status) {
    return QHttpServerResponse(
        QJsonObject{
            {"jsonrpc", "2.0"}, {"id", id},
            {"error", QJsonObject{{"code", code}, {"message", message}}}
        }, status);
}

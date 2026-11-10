#include "service/lsp/lspService.h"

#include <QJsonArray>
#include <QProcess>

#include "globals.h"
#include "util/uniCast.h"

// public
LspService::LspService(const QString &name, const QString &program, const QStringList &arguments, QObject *parent)
    : BaseService(name, parent),
      m_process(new QProcess(this)) {
    m_process->setProgram(program);
    m_process->setArguments(arguments);
    connect(m_process, &QProcess::readyRead, this, &LspService::parser);
}

LspService::~LspService() {
}

void LspService::initialize() {
    m_process->start();
    if (!m_process->waitForStarted()) {
        detailSet(m_process->errorString().toHtmlEscaped());
        qDebug() << "failed to start language server";
        return;
    }
    const QString rootUriStr = g_workspaceUrl.toString();
    const QJsonObject initializeParams{
        {"processId", QCoreApplication::applicationPid()},
        {"rootUri", rootUriStr},
        {"capabilities", QJsonObject{}}
    };
    jsonRequest("initialize", initializeParams);
    // wait for lsp initialized notification
    QEventLoop loop{};
    connect(this, &LspService::initialized, &loop, &QEventLoop::quit);
    loop.exec();
    jsonNotification("initialized", QJsonObject{});
}

void LspService::shutdown() {
    QEventLoop loop{};
    if (m_process->state() == QProcess::NotRunning) return;
    // wait for lsp exit notification
    connect(this, &LspService::shutdowned, &loop, &QEventLoop::quit);
    jsonRequest("shutdown", QJsonObject{});
    loop.exec();
    // wait for process exit
    connect(m_process, &QProcess::finished, this, [&loop] { loop.quit(); });
    jsonNotification("exit", QJsonObject{});
    loop.exec();
}

void LspService::jsonRequest(const QString &method, const QJsonObject &params) {
    const QJsonObject textDocument = params["textDocument"].toObject();
    const auto url = QUrl(textDocument["uri"].toString());
    m_methods.insert(m_id, method);
    m_urls.insert(m_id, url);
    const QJsonObject msg = {
        {"jsonrpc", "2.0"},
        {"id", m_id},
        {"method", method},
        {"params", params}
    };
    // qDebug() << m_methods;
    m_id++;
    const QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n";
    m_process->write(header);
    m_process->write(data);
    // qDebug() << msg;
}

void LspService::jsonNotification(const QString &method, const QJsonObject &params) const {
    const QJsonObject msg = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    const QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n";
    m_process->write(header);
    m_process->write(data);
    // qDebug() << msg;
}

// private
void LspService::jsonResponse(const QJsonValue &id, const QJsonValue &result) const {
    const QJsonObject msg = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
    const QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n";
    m_process->write(header);
    m_process->write(data);
}

void LspService::jsonError(const QJsonValue &id, const int code, const QString &message) const {
    const QJsonObject msg = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {
            "error", QJsonObject{
                {"code", code},
                {"message", message}
            }
        }
    };
    const QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n";
    m_process->write(header);
    m_process->write(data);
}

void LspService::parser() {
    // append to buffer
    m_buffer.append(m_process->readAllStandardOutput());
    while (true) {
        // extract header to get pack length
        const long long headerStartIndex = m_buffer.indexOf("Content-Length: ") + 16;
        const long long headerEndIndex = m_buffer.indexOf("\r\n\r\n");
        if (headerEndIndex == -1) break;
        const auto lengthBytes = m_buffer.mid(headerStartIndex, headerEndIndex - headerStartIndex);
        const auto length = lengthBytes.toInt();
        if (m_buffer.size() < headerEndIndex + 4 + length) break;
        const auto dataBytes = m_buffer.mid(headerEndIndex + 4, lengthBytes.toInt());
        const auto json = QJsonDocument::fromJson(dataBytes).object();
        // qDebug() << json;
        m_buffer.remove(0, headerEndIndex + 4 + length);
        if (json.contains("method")) {
            const auto method = json["method"].toString();
            const auto params = json["params"].toObject();
            // lsp request
            if (json.contains("id")) {
                const auto id = json["id"];
                if (method == "workspace/applyEdit") {
                    jsonResponse(id, QJsonObject{
                                     {"applied", false},
                                     {"failureReason", "Workspace edits are not supported."}
                                 });
                } else if (method == "window/workDoneProgress/create") {
                    jsonResponse(id, QJsonValue(QJsonValue::Null));
                } else {
                    jsonError(id, -32601, "Method not found");
                }
            }
            // lsp notification
            else {
                if (method == "textDocument/publishDiagnostics") {
                    // publish diagnostics notification
                    const auto diagnostics = params["diagnostics"].toArray();
                    const LUrl uri = params["uri"].toString();
                    const auto documentUrl = uni_cast<QUrl>(uri);
                    emit notificationDiagnostics(documentUrl, diagnostics);
                } else if (method == "$/hello") {
                    // hello notification
                    // qDebug() << json;
                } else if (method == "$/progress") {
                    // progress notification
                    // qDebug() << json;
                    // const auto params = json["params"].toObject();
                    // const auto token = params["token"].toInt();
                    // const auto value = params["value"].toObject();
                    // if (token == 2) {
                    //     const int percentage = value["percentage"].toInt(100);
                    //     m_progressDialog->setProperty("create2", percentage / 100.0);
                    // } else if (token == 3) {
                    //     const int percentage = value["percentage"].toInt(100);
                    //     m_progressDialog->setProperty("create3", percentage / 100.0);
                    // }
                } else if (method == "window/logMessage") {
                    // log message notification
                    // qDebug() << json;
                } else {
                    qDebug() << "unknown lsp pack";
                    qDebug() << json;
                }
            }
        } else if (json.contains("id")) {
            // lsp response
            const auto id = json["id"].toInt();
            const auto method = m_methods[id];
            const auto documentUrl = m_urls[id];
            m_methods.remove(id);
            m_urls.remove(id);
            eventAppend(method, json);
            if (method == "initialize") {
                // initialize response
                // qDebug() << json;
                const auto serverInfo = json["result"].toObject().value("serverInfo").toObject();
                QString detail{};
                if (const auto name = serverInfo.value("name").toString(); !name.isEmpty()) {
                    detail = QString("<b>%1</b> %2").arg(tr("Server:"), name.toHtmlEscaped());
                    if (const auto version = serverInfo.value("version").toString(); !version.isEmpty())
                        detail += QString("<br><b>%1</b> %2").arg(tr("Version:"), version.toHtmlEscaped());
                }
                detailSet(detail);
                emit initialized(json);
            } else if (method == "shutdown") {
                // shutdown response
                // qDebug() << json;
                emit shutdowned();
            } else if (method == "textDocument/codeAction") {
                // code action response
                // if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseCodeAction(documentUrl, result);
            } else if (method == "textDocument/completion") {
                // completion response
                if (!json["result"].isObject()) return; // null result
                const auto result = json["result"].toObject();
                const auto items = result["items"].toArray();
                emit responseCompletion(documentUrl, items);
            } else if (method == "textDocument/definition") {
                // definition response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseDefinition(documentUrl, result);
            } else if (method == "textDocument/documentHighlight") {
                // document highlight response
                // if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseDocumentHighlight(documentUrl, result);
            } else if (method == "textDocument/documentSymbol") {
                // document symbol response
                // if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseDocumentSymbol(documentUrl, result);
            } else if (method == "textDocument/foldingRange") {
                // folding range response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseFoldingRange(documentUrl, result);
            } else if (method == "textDocument/formatting") {
                // formatting response
                if (!json["result"].isArray()) {
                    emit responseFormatting(documentUrl, QString());
                } else {
                    const auto result = json["result"].toArray();
                    const auto newText = result[0]["newText"].toString();
                    emit responseFormatting(documentUrl, newText);
                }
            } else if (method == "textDocument/hover") {
                // hover response
                if (!json["result"].isObject()) {
                    // null result
                    emit responseHover(documentUrl, QString());
                } else {
                    const auto result = json["result"].toObject();
                    const auto contents = result["contents"].toObject();
                    const auto value = contents["value"].toString();
                    emit responseHover(documentUrl, value);
                }
            } else if (method == "textDocument/implementation") {
                // implementation response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseImplementation(documentUrl, result);
            } else if (method == "textDocument/onTypeFormatting") {
                // on type formatting response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                const auto newText = result[0].toObject();
                emit responseOnTypeFormatting(documentUrl, newText);
            } else if (method == "textDocument/prepareRename") {
                if (!json["result"].isObject()) return; // null result
                const auto result = json["result"].toObject();
                const auto oldName = result["placeholder"].toString();
                emit responsePrepareRename(documentUrl, oldName);
            } else if (method == "textDocument/rangeFormatting") {
                // range formatting response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                const auto newText = result[0]["newText"].toString();
                emit responseRangeFormatting(documentUrl, newText);
            } else if (method == "textDocument/references") {
                // references response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseReferences(documentUrl, result);
            } else if (method == "textDocument/rename") {
                // rename response
                if (!json["result"].isObject()) return; // null result
                const auto workspaceEdit = json["result"].toObject();
                emit responseRename(documentUrl, workspaceEdit);
            } else if (method == "textDocument/semanticTokens/full") {
                // semanticTokens response
                if (!json["result"].isObject()) return; // null result
                const auto result = json["result"].toObject();
                const auto data = result["data"].toArray();
                emit responseSemanticTokens(documentUrl, data);
            } else if (method == "textDocument/signatureHelp") {
                // signatureHelp response
                if (!json["result"].isObject()) return; // null result
                const auto result = json["result"].toObject();
                const auto signatures = result["signatures"].toArray();
                emit responseSignatureHelp(documentUrl, signatures);
            } else if (method == "textDocument/typeDefinition") {
                // typeDefinition response
                if (!json["result"].isArray()) return; // null result
                const auto result = json["result"].toArray();
                emit responseTypeDefinition(documentUrl, result);
            } else {
                qDebug() << "unknown lsp pack";
                qDebug() << json;
            }
        } else {
            qDebug() << "unknown lsp pack";
            qDebug() << json;
        }
        if (m_buffer.size() == 0) break;
    }
}

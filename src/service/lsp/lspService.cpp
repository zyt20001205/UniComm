#include "service/lsp/lspService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

#include "service/lsp/languageServer.h"

// public
LspService::LspService(QObject *parent)
    : QObject(parent) {
    const QDir rootDir(QCoreApplication::applicationDirPath());
    serverAdd("lua",
        rootDir.absoluteFilePath("lua-language-server/bin/lua-language-server.exe"),
        {"--configpath", rootDir.absoluteFilePath("lua-language-server/meta/3rd/UniComm/.luarc.json")});
}

LspService::~LspService() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] lsp service destructed").arg(timestamp);
}

void LspService::shutdown() {
    for (const auto &session: m_session) {
        session.server->shutdown();
    }
}

void LspService::jsonRequest(const QString &method, const QJsonObject &params) {
    const QJsonObject textDocument = params["textDocument"].toObject();
    const QUrl url(textDocument["uri"].toString());
    const QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();

    const auto it = m_session.find(suffix);
    if (it != m_session.end()) it.value().server->jsonRequest(method, params);
}

void LspService::jsonNotification(const QString &method, const QJsonObject &params) const {
    // workspace notification
    if (method.startsWith("workspace/")) {
        for (const auto &session: m_session) {
            session.server->jsonNotification(method, params);
        }
        return;
    }
    // file notification
    const QJsonObject textDocument = params["textDocument"].toObject();
    const QUrl url(textDocument["uri"].toString());
    const QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();
    const auto it = m_session.find(suffix);
    if (it != m_session.end()) it.value().server->jsonNotification(method, params);
}

// private
void LspService::serverAdd(const QString &suffix, const QString &process, const QStringList &arguments) {
    auto *server = new LanguageServer(process, arguments, this);
    connect(server, &LanguageServer::notificationDiagnostics, this, &LspService::notificationDiagnostics);
    connect(server, &LanguageServer::responseCodeAction, this, &LspService::responseCodeAction);
    connect(server, &LanguageServer::responseCompletion, this, &LspService::responseCompletion);
    connect(server, &LanguageServer::responseDefinition, this, &LspService::responseDefinition);
    connect(server, &LanguageServer::responseDocumentHighlight, this, &LspService::responseDocumentHighlight);
    connect(server, &LanguageServer::responseDocumentSymbol, this, &LspService::responseDocumentSymbol);
    connect(server, &LanguageServer::responseFoldingRange, this, &LspService::responseFoldingRange);
    connect(server, &LanguageServer::responseFormatting, this, &LspService::responseFormatting);
    connect(server, &LanguageServer::responseHover, this, &LspService::responseHover);
    connect(server, &LanguageServer::responseImplementation, this, &LspService::responseImplementation);
    connect(server, &LanguageServer::responseOnTypeFormatting, this, &LspService::responseOnTypeFormatting);
    connect(server, &LanguageServer::responsePrepareRename, this, &LspService::responsePrepareRename);
    connect(server, &LanguageServer::responseRangeFormatting, this, &LspService::responseRangeFormatting);
    connect(server, &LanguageServer::responseReferences, this, &LspService::responseReferences);
    connect(server, &LanguageServer::responseRename, this, &LspService::responseRename);
    connect(server, &LanguageServer::responseSemanticTokens, this, &LspService::responseSemanticTokens);
    connect(server, &LanguageServer::responseSignatureHelp, this, &LspService::responseSignatureHelp);
    connect(server, &LanguageServer::responseTypeDefinition, this, &LspService::responseTypeDefinition);
    connect(server, &LanguageServer::initialized, this, [this, suffix, server](const QJsonObject &params) {
        LanguageServerSession session{};
        session.server = server;
        const auto result = params["result"].toObject();
        const auto capabilities = result["capabilities"].toObject();
        // completion trigger
        for (const auto &value: capabilities["completionProvider"].toObject()["triggerCharacters"].toArray()) {
            const auto trigger = value.toString();
            if (!trigger.isEmpty()) session.completionTrigger.insert(trigger.front());
        }
        // format trigger
        {
            const auto trigger = capabilities["documentOnTypeFormattingProvider"].toObject()["firstTriggerCharacter"].toString();
            if (!trigger.isEmpty()) session.formattingTrigger.insert(trigger.front());
        }
        // signature help trigger
        for (const auto &value: capabilities["signatureHelpProvider"].toObject()["triggerCharacters"].toArray()) {
            const auto trigger = value.toString();
            if (!trigger.isEmpty()) session.signatureHelpTrigger.insert(trigger.front());
        }
        m_session.insert(suffix.toLower(), session);
    });
    server->initialize();
}

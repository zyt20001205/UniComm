#include "service/lsp/lspManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

// public
LspManager::LspManager(QObject *parent)
    : QObject(parent) {
    const QDir rootDir(QCoreApplication::applicationDirPath());
    serverAdd("lua", tr("Lua Language Server"),
        rootDir.absoluteFilePath("lua-language-server/bin/lua-language-server.exe"),
        {"--configpath", rootDir.absoluteFilePath("lua-language-server/meta/3rd/UniComm/.luarc.json")});
}

LspManager::~LspManager() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] lsp manager destructed").arg(timestamp);
}

void LspManager::shutdown() {
    for (const auto &session: m_session) {
        session.server->shutdown();
    }
}

QList<LspService *> LspManager::serversGet() const {
    return findChildren<LspService *>(Qt::FindDirectChildrenOnly);
}

void LspManager::jsonRequest(const QString &method, const QJsonObject &params) {
    const QJsonObject textDocument = params["textDocument"].toObject();
    const QUrl url(textDocument["uri"].toString());
    const QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();

    const auto it = m_session.find(suffix);
    if (it != m_session.end()) it.value().server->jsonRequest(method, params);
}

void LspManager::jsonNotification(const QString &method, const QJsonObject &params) const {
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
void LspManager::serverAdd(const QString &suffix, const QString &name, const QString &process, const QStringList &arguments) {
    auto *server = new LspService(name, process, arguments, this);
    connect(server, &LspService::notificationDiagnostics, this, &LspManager::notificationDiagnostics);
    connect(server, &LspService::responseCodeAction, this, &LspManager::responseCodeAction);
    connect(server, &LspService::responseCompletion, this, &LspManager::responseCompletion);
    connect(server, &LspService::responseDefinition, this, &LspManager::responseDefinition);
    connect(server, &LspService::responseDocumentHighlight, this, &LspManager::responseDocumentHighlight);
    connect(server, &LspService::responseDocumentSymbol, this, &LspManager::responseDocumentSymbol);
    connect(server, &LspService::responseFoldingRange, this, &LspManager::responseFoldingRange);
    connect(server, &LspService::responseFormatting, this, &LspManager::responseFormatting);
    connect(server, &LspService::responseHover, this, &LspManager::responseHover);
    connect(server, &LspService::responseImplementation, this, &LspManager::responseImplementation);
    connect(server, &LspService::responseOnTypeFormatting, this, &LspManager::responseOnTypeFormatting);
    connect(server, &LspService::responsePrepareRename, this, &LspManager::responsePrepareRename);
    connect(server, &LspService::responseRangeFormatting, this, &LspManager::responseRangeFormatting);
    connect(server, &LspService::responseReferences, this, &LspManager::responseReferences);
    connect(server, &LspService::responseRename, this, &LspManager::responseRename);
    connect(server, &LspService::responseSemanticTokens, this, &LspManager::responseSemanticTokens);
    connect(server, &LspService::responseSignatureHelp, this, &LspManager::responseSignatureHelp);
    connect(server, &LspService::responseTypeDefinition, this, &LspManager::responseTypeDefinition);
    connect(server, &LspService::initialized, this, [this, suffix, server](const QJsonObject &params) {
        Session session{};
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

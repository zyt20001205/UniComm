#ifndef UNICOMM_LSPSERVICE_H
#define UNICOMM_LSPSERVICE_H

#include <QStringList>

#include "languageServer.h"

class LspService final : public QObject {
    Q_OBJECT

public:
    explicit LspService(QObject *parent = nullptr);

    ~LspService() override;

    void shutdown();

    void jsonRequest(const QString &method, const QJsonObject &params);

    void jsonNotification(const QString &method, const QJsonObject &params) const;

signals:
    void notificationDiagnostics(const QUrl &documentUrl, const QJsonArray &diagnostics);

    void responseCodeAction(const QUrl &documentUrl, const QJsonArray &result);

    void responseCompletion(const QUrl &documentUrl, const QJsonArray &items);

    void responseDefinition(const QUrl &documentUrl, const QJsonArray &ranges);

    void responseDocumentHighlight(const QUrl &documentUrl, const QJsonArray &result);

    void responseDocumentSymbol(const QUrl &documentUrl, const QJsonArray &result);

    void responseFoldingRange(const QUrl &documentUrl, const QJsonArray &result);

    void responseFormatting(const QUrl &documentUrl, const QString &newText);

    void responseHover(const QUrl &documentUrl, const QString &message);

    void responseImplementation(const QUrl &documentUrl, const QJsonArray &ranges);

    void responseOnTypeFormatting(const QUrl &documentUrl, const QJsonObject &newText);

    void responsePrepareRename(const QUrl &documentUrl, const QString &oldName);

    void responseRangeFormatting(const QUrl &documentUrl, const QString &newText);

    void responseReferences(const QUrl &documentUrl, const QJsonArray &ranges);

    void responseRename(const QUrl &documentUrl, const QJsonObject &workspaceEdit);

    void responseSemanticTokens(const QUrl &documentUrl, const QJsonArray &data);

    void responseSignatureHelp(const QUrl &documentUrl, const QJsonArray &signature);

    void responseTypeDefinition(const QUrl &documentUrl, const QJsonArray &ranges);

private:
    struct LanguageServerSession {
        LanguageServer *server{};
        QSet<QChar> completionTrigger{};
        QSet<QChar> formattingTrigger{};
        QSet<QChar> signatureHelpTrigger{};
    };

    void serverAdd(const QString &suffix, const QString &process, const QStringList &arguments = {});

    QHash<QString, LanguageServerSession> m_session{};
};

#endif //UNICOMM_LSPSERVICE_H

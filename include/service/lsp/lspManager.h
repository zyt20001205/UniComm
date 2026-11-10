#ifndef UNICOMM_LSPMANAGER_H
#define UNICOMM_LSPMANAGER_H

#include <QStringList>

#include "lspService.h"

class LspManager final : public QObject {
    Q_OBJECT

public:
    explicit LspManager(QObject *parent = nullptr);

    ~LspManager() override;

    void shutdown();

    [[nodiscard]] QList<LspService *> serversGet() const;

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
    struct Session {
        LspService *server{};
        QSet<QChar> completionTrigger{};
        QSet<QChar> formattingTrigger{};
        QSet<QChar> signatureHelpTrigger{};
    };

    void serverAdd(const QString &suffix, const QString &name, const QString &process, const QStringList &arguments = {});

    QHash<QString, Session> m_session{};
};

#endif //UNICOMM_LSPMANAGER_H

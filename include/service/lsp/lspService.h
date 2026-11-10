#ifndef UNICOMM_LSPSERVICE_H
#define UNICOMM_LSPSERVICE_H

#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>

#include "service/baseService.h"

class QProcess;

class LspService final : public BaseService {
    Q_OBJECT

public:
    explicit LspService(const QString &name, const QString &program, const QStringList &arguments, QObject *parent = nullptr);

    ~LspService() override;

    void initialize();

    void shutdown();

    void jsonRequest(const QString &method, const QJsonObject &params);

    void jsonNotification(const QString &method, const QJsonObject &params) const;

signals:
    void initialized(const QJsonObject &params);

    void shutdowned();

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
    void jsonResponse(const QJsonValue &id, const QJsonValue &result) const;

    void jsonError(const QJsonValue &id, int code, const QString &message) const;

    void parser();

    QProcess *m_process{};
    QByteArray m_buffer{};
    int m_id{0};
    QHash<int, QString> m_methods{};
    QHash<int, QUrl> m_urls{};
};

#endif //UNICOMM_LSPSERVICE_H

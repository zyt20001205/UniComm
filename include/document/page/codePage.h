#ifndef UNICOMM_CODEPAGE_H
#define UNICOMM_CODEPAGE_H

#include "documentPage.h"
#include "document/module/codeWidget.h"

class SymbolWidget;

class CodePage final : public DocumentPage {
    Q_OBJECT

public:
    explicit CodePage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~CodePage() override = default;

    void propertySet(const QVariantHash &objects) const;

    [[nodiscard]] ScintillaWidget *handler() const { return m_codeWidget->handler(); }

    [[nodiscard]] QString documentSave() override;

    [[nodiscard]] QVariantHash menuLoad(const QString &name) const;

    void menuCall(const QString &name) const;

    // public: lsp
    void diagnosticsNotification(const QJsonArray &diagnostics) const;

    void documentHighlightResponse(const QJsonArray &result) const;

    void documentSymbolResponse(const QJsonArray &result);

    void foldingRangeResponse(const QJsonArray &result) const;

    void formattingResponse(const QString &newText) const;

    void onTypeFormattingResponse(const QJsonObject &newText) const;

    void rangeFormattingResponse(const QString &newText) const;

    void renameResponse(const QJsonArray &changes) const;

    void semanticTokensResponse(const QJsonArray &data) const;

    // public: typo
    void spellCheckResponse(const QVariantList &typos) const;

signals:
    void changeSelection(const QHash<QString, int> &selection);

    void insertBreakpoint(const QUrl &documentUrl, int line, const QVariantHash &session);

    void removeBreakpoint(const QUrl &documentUrl, int line);

    void requestCompletion(const QUrl &documentUrl, int line, int character);

    void requestDefinition(const QUrl &documentUrl, int line, int character);

    void requestDocumentHighlight(const QUrl &documentUrl, int line, int character);

    void requestDocumentSymbol(const QUrl &documentUrl);

    void requestFoldingRange(const QUrl &documentUrl);

    void requestFormatting(const QUrl &documentUrl);

    void requestHover(const QUrl &documentUrl, int line, int character);

    void requestImplementation(const QUrl &documentUrl, int line, int character);

    void requestOnTypeFormatting(const QUrl &documentUrl, int line, int character);

    void requestPrepareRename(const QUrl &documentUrl, int line, int character);

    void requestReferences(const QUrl &documentUrl, int line, int character);

    void requestSemanticTokens(const QUrl &documentUrl);

    void requestSignatureHelp(const QUrl &documentUrl, int line, int character);

    void requestSpellCheck(const QUrl &documentUrl, const QString &script);

    void requestTypeDefinition(const QUrl &documentUrl, int line, int character);

    void notificationJson(const QString &method, const QJsonObject &params);

    void showDiagnostic(const QVariantHash &diagnosticSession, const QString &message);

protected:
    [[nodiscard]] bool documentModified() const override;

private:
    void savepointChange(bool status);

    CodeWidget *m_codeWidget{};
    SymbolWidget *m_symbolWidget{};

    QJsonArray m_symbol{};
};

#endif //UNICOMM_CODEPAGE_H

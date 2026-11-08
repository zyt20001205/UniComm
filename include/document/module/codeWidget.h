#ifndef UNICOMM_CODEWIDGET_H
#define UNICOMM_CODEWIDGET_H

#include <QJsonArray>

#include "editorWidget.h"
#include "ScintillaTypes.h"

class EditorWidget;

class CodeWidget final : public EditorWidget {
    Q_OBJECT

public:
    explicit CodeWidget(const QJsonObject &documentConfig, const QUrl &documentUrl, QWidget *parent = nullptr);

    ~CodeWidget() override = default;

    void propertySet(const QVariantHash &objects) override;

    [[nodiscard]] QString documentSave() override;

    [[nodiscard]] QVariantHash menuLoad(const QString &name) const;

    void menuCall(const QString &name) const;

    // public: lsp
    void diagnosticsNotification(const QJsonArray &diagnostics);

    void documentHighlightResponse(const QJsonArray &result) const;

    void foldingRangeResponse(const QJsonArray &result) const;

    void formattingResponse(const QString &newText) const;

    void onTypeFormattingResponse(const QJsonObject &newText) const;

    void rangeFormattingResponse(const QString &newText) const;

    void renameResponse(const QJsonArray &changes) const;

    void semanticTokensResponse(const QJsonArray &data);

    // public: typo
    void spellCheckResponse(const QVariantList &typos);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void insertBreakpoint(const QUrl &documentUrl, int line, const QVariantHash &session);

    void removeBreakpoint(const QUrl &documentUrl, int line);

    void notificationJson(const QString &method, const QJsonObject &params);

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

    void showDiagnostic(const QVariantHash &diagnosticSession, const QString &message);

    void showDocumentSymbol(int line, int character);

protected:
    void shortcutInit() override;

    void selectionChange() override;

    void contentChange() override;

    bool symbolPair(QChar ch) override;

    void indicatorInit() const override;

    void marginInit() const override;

    void markerInit() const override;

    void lexerInit() const override;

private:
    void charAdd(int ch);

    void marginClick(Scintilla::Position position, int mouseButton, Scintilla::KeyMod modifiers, int margin);

    void dwellChange();

    // private: file
    void breakpointGet() const;

    void breakpointSet(int line) const;

    // private: lsp
    void didOpenNotification();

    void didChangeNotification();

    void didSaveNotification();

    void didCloseNotification();

    void completionRequest();

    void definitionRequest();

    void documentHighlightRequest();

    void documentSymbolRequest();

    void foldingRangeRequest();

    void formattingRequest();

    void hoverRequest();

    void implementationRequest();

    void prepareRenameRequest();

    void referencesRequest();

    void onTypeFormattingRequest();

    void semanticTokensRequest();

    void signatureHelpRequest();

    void typeDefinitionRequest();

    // private: typo
    void spellCheckRequest();

    // private: misc
    void commentToggle();

    [[nodiscard]] bool navigable(Scintilla::Position position) const;

    void navigationToggle(Scintilla::Position position = -1) const;

    QObject *m_toolTip{};
    QObject *m_breakpointEditDialog{};
    QObject *m_editorMenu{};

    QTimer *m_dwellTimer{};
    QSet<QChar> m_completionTrigger{};
    QSet<QChar> m_formattingTrigger{};
    QSet<QChar> m_signatureHelpTrigger{};

    int m_version = 1;
    QJsonArray m_diagnostic{};
    QVariantList m_typo{};
};

#endif //UNICOMM_CODEWIDGET_H

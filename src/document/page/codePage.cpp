#include "document/page/codePage.h"

#include <QVBoxLayout>

#include "globals.h"
#include "document/module/scintillaWidget.h"
#include "document/module/symbolWidget.h"

// public
CodePage::CodePage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_codeWidget(new CodeWidget(documentConfig, documentUrl, this)),
      m_symbolWidget(new SymbolWidget(this)) {
    auto *widget = new QWidget(); // NOLINT
    setWidget(widget);
    auto *layout = new QVBoxLayout(widget); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_codeWidget);
    layout->addWidget(m_symbolWidget);

    connect(m_codeWidget, &EditorWidget::appendLog, this, &CodePage::appendLog);
    connect(m_codeWidget, &EditorWidget::changeSavepoint, this, &CodePage::savepointChange);
    connect(m_codeWidget, &EditorWidget::changeSelection, this, &CodePage::changeSelection);
    connect(m_codeWidget, &CodeWidget::insertBreakpoint, this, &CodePage::insertBreakpoint);
    connect(m_codeWidget, &CodeWidget::notificationJson, this, &CodePage::notificationJson);
    connect(m_codeWidget, &CodeWidget::requestCompletion, this, &CodePage::requestCompletion);
    connect(m_codeWidget, &CodeWidget::requestDefinition, this, &CodePage::requestDefinition);
    connect(m_codeWidget, &CodeWidget::requestDocumentHighlight, this, &CodePage::requestDocumentHighlight);
    connect(m_codeWidget, &CodeWidget::requestDocumentSymbol, this, &CodePage::requestDocumentSymbol);
    connect(m_codeWidget, &CodeWidget::requestFoldingRange, this, &CodePage::requestFoldingRange);
    connect(m_codeWidget, &CodeWidget::requestFormatting, this, &CodePage::requestFormatting);
    connect(m_codeWidget, &CodeWidget::requestHover, this, &CodePage::requestHover);
    connect(m_codeWidget, &CodeWidget::requestImplementation, this, &CodePage::requestImplementation);
    connect(m_codeWidget, &CodeWidget::requestOnTypeFormatting, this, &CodePage::requestOnTypeFormatting);
    connect(m_codeWidget, &CodeWidget::requestPrepareRename, this, &CodePage::requestPrepareRename);
    connect(m_codeWidget, &CodeWidget::requestReferences, this, &CodePage::requestReferences);
    connect(m_codeWidget, &CodeWidget::requestSemanticTokens, this, &CodePage::requestSemanticTokens);
    connect(m_codeWidget, &CodeWidget::requestSignatureHelp, this, &CodePage::requestSignatureHelp);
    connect(m_codeWidget, &CodeWidget::requestSpellCheck, this, &CodePage::requestSpellCheck);
    connect(m_codeWidget, &CodeWidget::requestTypeDefinition, this, &CodePage::requestTypeDefinition);
    connect(m_codeWidget, &CodeWidget::showDiagnostic, this, &CodePage::showDiagnostic);
    connect(m_codeWidget, &CodeWidget::showDocumentSymbol, this, [this](const int line, const int character) { m_symbolWidget->documentSymbolShow(m_symbol, line, character); });
    connect(m_symbolWidget, &SymbolWidget::appendLog, this, &CodePage::appendLog);
    connect(m_symbolWidget, &SymbolWidget::setFocus, handler(), &ScintillaWidget::focusSet);
    connect(m_symbolWidget, &SymbolWidget::setIndex, handler(), &ScintillaWidget::indexSet);
    connect(m_symbolWidget, &SymbolWidget::fillIndicator, handler(), &ScintillaWidget::indicatorFill);
}

void CodePage::propertySet(const QVariantHash &objects) const {
    m_codeWidget->propertySet(QVariantHash{
        {"theme", objects["theme"]},
        {"mainWindowToast", objects["mainWindowToast"]},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"breakpointModuleEditDialog", objects["breakpointModuleEditDialog"]},
        {"fileModulePropertyDialog", objects["fileModulePropertyDialog"]},
        {"documentModuleGotoDialog", objects["documentModuleGotoDialog"]},
        {"documentModuleEditorMenu", objects["documentModuleEditorMenu"]}
    });
    m_symbolWidget->propertySet(QVariantHash{
        {"mainWindowToolTip", objects["mainWindowToolTip"]}
    });
}

QString CodePage::documentSave() {
    return m_codeWidget->documentSave();
}

QVariantHash CodePage::menuLoad(const QString &name) const {
    return m_codeWidget->menuLoad(name);
}

void CodePage::menuCall(const QString &name) const {
    m_codeWidget->menuCall(name);
}

void CodePage::diagnosticsNotification(const QJsonArray &diagnostics) const {
    m_codeWidget->diagnosticsNotification(diagnostics);
}

void CodePage::documentHighlightResponse(const QJsonArray &result) const {
    m_codeWidget->documentHighlightResponse(result);
}

void CodePage::documentSymbolResponse(const QJsonArray &result) {
    if (!m_documentUrl.toString().endsWith(".lua")) return;
    m_symbol = result;
}

void CodePage::foldingRangeResponse(const QJsonArray &result) const {
    m_codeWidget->foldingRangeResponse(result);
}

void CodePage::formattingResponse(const QString &newText) const {
    m_codeWidget->formattingResponse(newText);
}

void CodePage::onTypeFormattingResponse(const QJsonObject &newText) const {
    m_codeWidget->onTypeFormattingResponse(newText);
}

void CodePage::rangeFormattingResponse(const QString &newText) const {
    m_codeWidget->rangeFormattingResponse(newText);
}

void CodePage::renameResponse(const QJsonArray &changes) const {
    m_codeWidget->renameResponse(changes);
}

void CodePage::semanticTokensResponse(const QJsonArray &data) const {
    m_codeWidget->semanticTokensResponse(data);
}

void CodePage::spellCheckResponse(const QVariantList &typos) const {
    m_codeWidget->spellCheckResponse(typos);
}

// protected
bool CodePage::documentModified() const {
    return handler()->modifyGet();
}

// private
void CodePage::savepointChange(const bool status) {
    const QString pageName = title();
    if (status) {
        setTitle(pageName + "*");
    } else {
        setTitle(pageName.chopped(1));
    }
}

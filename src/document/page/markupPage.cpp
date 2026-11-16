#include "document/page/markupPage.h"

#include <QFileInfo>
#include <QSplitter>

#include "document/module/editorWidget.h"
#include "document/module/scintillaWidget.h"
#include "document/module/webviewWidget.h"
#include "util/uniCast.h"

// public
MarkupPage::MarkupPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_editorWidget(new EditorWidget(documentConfig, documentUrl)),
      m_webviewWidget(new WebviewWidget()) {
    auto *splitter = new QSplitter(Qt::Horizontal); // NOLINT
    splitter->addWidget(m_editorWidget);
    splitter->addWidget(m_webviewWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({600, 600});
    setWidget(splitter);
    previewUpdate();
    connect(m_editorWidget, &EditorWidget::appendLog, this, &MarkupPage::appendLog);
    connect(m_editorWidget, &EditorWidget::changeSavepoint, this, &MarkupPage::savepointChange);
    connect(m_editorWidget, &EditorWidget::changeSelection, this, &MarkupPage::changeSelection);
    connect(m_editorWidget, &EditorWidget::changeContent, this, &MarkupPage::previewUpdate);
}

void MarkupPage::propertySet(const QVariantHash &objects) const {
    m_editorWidget->propertySet(QVariantHash{
        {"mainWindowToast", objects["mainWindowToast"]},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"fileModulePropertyDialog", objects["fileModulePropertyDialog"]},
        {"documentModuleGotoDialog", objects["documentModuleGotoDialog"]}
    });
}

QString MarkupPage::documentSave() {
    return m_editorWidget->documentSave();
}

// protected
bool MarkupPage::documentModified() const {
    return handler()->modifyGet();
}

// private
void MarkupPage::previewUpdate() const {
    const auto text = handler()->textGet();
    if (QFileInfo(m_documentUrl.toLocalFile()).suffix().toLower() == "md") {
        m_webviewWidget->setHtml(uni_cast<QFullHtmlString>(text));
    } else {
        m_webviewWidget->setHtml(text);
    }
}

void MarkupPage::savepointChange(const bool status) {
    const QString pageName = title();
    if (status) {
        setTitle(pageName + "*");
    } else {
        setTitle(pageName.chopped(1));
    }
}

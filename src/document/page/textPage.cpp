#include "document/page/textPage.h"

#include <QDir>
#include <QFileInfo>
#include <QShortcut>

#include "globals.h"
#include "document/module/editorWidget.h"
#include "document/module/scintillaWidget.h"

// public
TextPage::TextPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_editorWidget(new EditorWidget(documentConfig, documentUrl, this)) {
    setWidget(m_editorWidget);
    connect(m_editorWidget, &EditorWidget::appendLog, this, &TextPage::appendLog);
    connect(m_editorWidget, &EditorWidget::changeSavepoint, this, &TextPage::savepointChange);
    connect(m_editorWidget, &EditorWidget::changeSelection, this, &TextPage::changeSelection);
}

void TextPage::propertySet(const QVariantHash &objects) const {
    m_editorWidget->propertySet(QVariantHash{
        {"mainWindowToast", objects["mainWindowToast"]},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"fileModulePropertyDialog", objects["fileModulePropertyDialog"]},
        {"documentModuleGotoDialog", objects["documentModuleGotoDialog"]}
    });
}

QString TextPage::documentSave() {
    return m_editorWidget->documentSave();
}

// protected
bool TextPage::documentModified() const {
    return handler()->modifyGet();
}

// private
void TextPage::savepointChange(const bool status) {
    const QString pageName = title();
    if (status) {
        setTitle(pageName + "*");
    } else {
        setTitle(pageName.chopped(1));
    }
}

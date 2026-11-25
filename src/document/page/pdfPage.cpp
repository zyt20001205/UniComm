#include "document/page/pdfPage.h"

#include <QPdfDocument>
#include <QQuickWidget>

#include "core/globalManager.h"

// public
PdfPage::PdfPage(const QJsonObject &, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_pdfWidget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_doc(new QPdfDocument(this)) {
    setWidget(m_pdfWidget);
    m_doc->load(m_documentUrl.toLocalFile());
}

void PdfPage::propertySet() {
    m_pdfWidget->setInitialProperties({{"source", m_documentUrl}});
    m_pdfWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_pdfWidget->setSource(QUrl("qrc:/qml/document/module/pdfWidget.qml"));
}

QString PdfPage::textGet(const int page) const {
    if (page < 0 || page >= m_doc->pageCount()) return {"out of range"};
    return m_doc->getAllText(page).text();
}

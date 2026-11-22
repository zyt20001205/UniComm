#include "document/page/pdfPage.h"

#include <QPdfDocument>
#include <QQmlContext>
#include <QQuickWidget>

// public
PdfPage::PdfPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_pdfWidget(new QQuickWidget()),
      m_doc(new QPdfDocument(this)) {
    setWidget(m_pdfWidget);
    m_doc->load(m_documentUrl.toLocalFile());
}

void PdfPage::propertySet() {
    m_pdfWidget->rootContext()->setContextProperty("pdfPage", this);
    m_pdfWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_pdfWidget->setSource(QUrl("qrc:/qml/document/module/pdfWidget.qml"));
}

void PdfPage::propertyGet(const QVariantMap &objects) {
    m_pdf = qvariant_cast<QObject *>(objects["pdf"]);
    m_pdf->setProperty("source", m_documentUrl);
}

QString PdfPage::textGet(const int page) const {
    if (page < 0 || page >= m_doc->pageCount()) return {"out of range"};
    return m_doc->getAllText(page).text();
}

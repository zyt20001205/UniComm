#include "document/page/imagePage.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"

// public
ImagePage::ImagePage(const QJsonObject &, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_imageWidget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_imageWidget);
}

void ImagePage::propertySet() {
    m_imageWidget->setInitialProperties({{"source", m_documentUrl}});
    m_imageWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_imageWidget->setSource(QUrl("qrc:/qml/util/ImageViewer.qml"));
}

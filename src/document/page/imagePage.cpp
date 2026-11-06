#include "document/page/imagePage.h"

#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"

// public
ImagePage::ImagePage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_imageWidget(new QQuickWidget()) {
    setWidget(m_imageWidget);
}

void ImagePage::propertySet(const QVariantHash &objects) {
    m_imageWidget->rootContext()->setContextProperty("global", g_globalManager);
    m_imageWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_imageWidget->setSource(QUrl("qrc:/qml/util/ImageViewer.qml"));
    m_imageWidget->rootObject()->setProperty("source", m_documentUrl);
}

#include "document/page/webPage.h"

#include <QIcon>

#include "document/module/webviewWidget.h"

// public
WebPage::WebPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_webviewWidget(new WebviewWidget()) {
    setTitle(documentUrl.host());
    setIcon(QIcon(":/icon/fileTypeHttp.svg"));
    setWidget(m_webviewWidget);
    m_webviewWidget->navigate(documentUrl);
}

void WebPage::propertySet() {
}

#include "document/page/welcomePage.h"

#include <QQuickWidget>

#include "core/globalManager.h"

// public
WelcomePage::WelcomePage()
    : DockWidget("welcome"),
      m_welcomeWidget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_welcomeWidget);
}

void WelcomePage::propertySet() {
    m_welcomeWidget->setInitialProperties({{"welcomePage", QVariant::fromValue(this)}});
    m_welcomeWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_welcomeWidget->setSource(QUrl("qrc:/qml/document/page/welcomePage.qml"));
}

void WelcomePage::workspaceOpen() {
    emit openWorkspace();
}

void WelcomePage::documentOpen(const QUrl &documentUrl) {
    emit openDocument(documentUrl);
}

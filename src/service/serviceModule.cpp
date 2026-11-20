#include "service/serviceModule.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/grepService.h"
#include "service/lsp/lspManager.h"
#include "service/mcpService.h"
#include "service/network/networkService.h"

ServiceModule::ServiceModule()
    : DockWidget("Services"),
      m_lspManager(new LspManager(this)),
      m_mcpService(new McpService(this)),
      m_grepService(new GrepService(this)),
      m_networkService(new NetworkService(this)),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);
}

void ServiceModule::propertySet() {
    m_widget->setInitialProperties({{"serviceModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/service/serviceModule.qml"));
}

QVariantList ServiceModule::servicesGet() const {
    QVariantList services{
        QVariant::fromValue<BaseService *>(m_mcpService),
        QVariant::fromValue<BaseService *>(m_grepService),
        QVariant::fromValue<BaseService *>(m_networkService)
    };
    for (auto *server: m_lspManager->serversGet()) {
        services.append(QVariant::fromValue<BaseService *>(server));
    }
    return services;
}

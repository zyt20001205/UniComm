#include "service/serviceModule.h"

#include <QQmlContext>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/lsp/lspManager.h"
#include "service/mcp/mcpService.h"

ServiceModule::ServiceModule()
    : DockWidget("Services"),
      m_lspManager(new LspManager(this)),
      m_mcpService(new McpService(this)),
      m_widget(new QQuickWidget()) {
    setWidget(m_widget);
}

void ServiceModule::propertySet(const QVariantHash &objects) {
    m_widget->rootContext()->setContextProperty("serviceModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainToolTip", qvariant_cast<QObject *>(objects["mainWindowToolTip"]));
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/service/serviceModule.qml"));
}

LspManager *ServiceModule::lspManagerGet() const {
    return m_lspManager;
}

QVariantList ServiceModule::servicesGet() const {
    QVariantList services{QVariant::fromValue<BaseService *>(m_mcpService)};
    for (auto *server: m_lspManager->serversGet()) {
        services.append(QVariant::fromValue<BaseService *>(server));
    }
    return services;
}

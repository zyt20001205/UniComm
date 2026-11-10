#include "service/serviceModule.h"

#include <QQmlContext>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/lsp/lspService.h"
#include "service/mcp/mcpService.h"

ServiceModule::ServiceModule()
    : DockWidget("Services"),
      m_lspService(new LspService(this)),
      m_services{QVariant::fromValue<BaseService *>(new McpService(this))},
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

LspService *ServiceModule::lspServiceGet() const {
    return m_lspService;
}

QVariantList ServiceModule::servicesGet() const {
    return m_services;
}

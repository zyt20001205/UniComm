#include "document/module/resolveWidget.h"

#include <QColor>
#include <QQmlContext>
#include <QQuickItem>

#include "globals.h"
#include "core/globalManager.h"

// public
ResolveWidget::ResolveWidget(QWidget *parent)
    : QQuickWidget(parent) {
    setFixedSize(240, 36);
    setClearColor(Qt::transparent);
    setAttribute(Qt::WA_AlwaysStackOnTop);
}

void ResolveWidget::propertySet() {
    rootContext()->setContextProperty("resolveWidget", this);
    rootContext()->setContextProperty("global", g_globalManager);

    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/document/module/resolveWidget.qml"));
    m_root = rootObject();
}

void ResolveWidget::propertyGet(const QVariantMap &objects) {
    m_resolveStatLabel = qvariant_cast<QObject *>(objects["resolveStatLabel"]);
    m_resolveFinishButton = qvariant_cast<QObject *>(objects["resolveFinishButton"]);
}

void ResolveWidget::resolveStat(const int conflicts) const {
    m_root->setProperty("conflicts", conflicts);
}

void ResolveWidget::resolveFinish() {
    emit finishResolve();
}

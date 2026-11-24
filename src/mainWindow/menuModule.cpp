#include "mainWindow/menuModule.h"

#include <QQmlEngine>

#include "globals.h"
#include "core/globalManager.h"

// public
MenuModule::MenuModule(QWidget *parent)
    : QQuickWidget(g_globalManager->engineGet(), parent) {
    setFixedHeight(24);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

MenuModule::~MenuModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] menu module destructed").arg(timestamp);
}

void MenuModule::propertySet() {
    setInitialProperties({{"menuModule", QVariant::fromValue(this)}});
    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/mainWindow/menuModule.qml"));
}

QAction *MenuModule::toggleActionGet(KDDockWidgets::QtWidgets::DockWidget *dock) const {
    auto *action = dock->toggleAction();
    QJSEngine::setObjectOwnership(action, QJSEngine::CppOwnership);
    return action;
}

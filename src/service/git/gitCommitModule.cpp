#include "service/git/gitCommitModule.h"

#include <QQuickWidget>

#include "core/globalManager.h"

// public
GitCommitModule::GitCommitModule()
    : DockWidget("Commit") {
    auto *widget = new QQuickWidget(g_globalManager->engineGet(), nullptr);
    setWidget(widget);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget->setSource(QUrl("qrc:/qml/service/git/gitCommitModule.qml"));
}

void GitCommitModule::commitShow() {
    if (!isOpen()) open();
    raise();
}

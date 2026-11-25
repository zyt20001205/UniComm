#include "document/module/resolveWidget.h"

#include <QColor>

#include "globals.h"
#include "core/globalManager.h"

// public
ResolveWidget::ResolveWidget(QWidget *parent)
    : QQuickWidget(g_globalManager->engineGet(), parent) {
    setFixedSize(240, 36);
    setClearColor(Qt::transparent);
    setAttribute(Qt::WA_AlwaysStackOnTop);
}

void ResolveWidget::propertySet() {
    setInitialProperties({{"resolveWidget", QVariant::fromValue(this)}});
    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/document/module/resolveWidget.qml"));
}

void ResolveWidget::resolveStat(const int conflicts) {
    if (m_conflicts == conflicts) return;
    m_conflicts = conflicts;
    emit conflictsChanged();
}

void ResolveWidget::resolveFinish() {
    emit finishResolve();
}

#include "document/assistant/positionWidget.h"

#include <QQmlComponent>
#include <QTimer>
#include <windows.h>

#include "globals.h"
#include "core/globalManager.h"

// public
PositionWidget::PositionWidget(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)) {
    m_timer->setInterval(16); // 60Hz
    connect(m_timer, &QTimer::timeout, this, &PositionWidget::positionUpdate);
    connect(this, &PositionWidget::visibleChanged, this, [this] {
        if (m_visible) m_timer->start();
        else m_timer->stop();
    });
}

void PositionWidget::propertySet() {
    QQmlComponent component(g_globalManager->engineGet(), QUrl("qrc:/qml/document/assistant/positionWidget.qml"));
    if (auto *root = component.createWithInitialProperties({{"positionWidget", QVariant::fromValue(this)}})) root->setParent(this);
    else qWarning().noquote() << component.errorString();
}

void PositionWidget::positionShow(const QVariantMap &positionSession) {
    m_positionSession = positionSession;
    positionUpdate();
    emit showPosition();
}

void PositionWidget::positionHide() {
    if (m_visible) emit hidePosition();
}

void PositionWidget::textReplace() {
    emit setText(
        m_positionSession["documentUrl"].toUrl(),
        m_text,
        m_positionSession["line"].toInt(),
        m_positionSession["character"].toInt(),
        m_positionSession["line"].toInt(),
        m_positionSession["character"].toInt());
    positionHide();
}

// private
void PositionWidget::positionUpdate() {
    if (g_terminating) {
        m_timer->stop();
        return;
    }
    POINT physicalPos{};
    GetCursorPos(&physicalPos);
    m_text = QString("%1, %2").arg(physicalPos.x).arg(physicalPos.y);
    emit setPosition(QCursor::pos(), m_text);
}

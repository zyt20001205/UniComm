#include "document/assistant/positionWidget.h"

#include <QTimer>
#include <windows.h>

#include "globals.h"

// public
PositionWidget::PositionWidget(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)) {
    m_timer->setInterval(16); // 60Hz
    connect(m_timer, &QTimer::timeout, this, [this] {
        if (g_terminating) {
            m_timer->stop();
            return;
        }
        m_tooltip->setProperty("position", QCursor::pos());
        POINT physicalPos{};
        GetCursorPos(&physicalPos);
        m_tooltip->setProperty("text", QString("%1, %2").arg(QString::number(physicalPos.x), QString::number(physicalPos.y)));
    });
}

void PositionWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModulePositionTooltip"]);
}

bool PositionWidget::isVisible() const {
    if (!m_tooltip) return false;
    return m_tooltip->property("visible").toBool();
}

void PositionWidget::positionShow(const QVariantMap &positionSession) {
    m_positionSession = positionSession;
    m_timer->start();
    QMetaObject::invokeMethod(m_tooltip, "open");
}

void PositionWidget::positionHide() const {
    m_timer->stop();
    QMetaObject::invokeMethod(m_tooltip, "close");
}

void PositionWidget::textReplace() {
    emit setText(
        m_positionSession["documentUrl"].toUrl(),
        m_tooltip->property("text").toString(),
        m_positionSession["line"].toInt(),
        m_positionSession["character"].toInt(),
        m_positionSession["line"].toInt(),
        m_positionSession["character"].toInt());
    positionHide();
}

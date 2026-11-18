#include "debug/debugModule.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"

// public
DebugModule::DebugModule()
    : DockWidget("Debug"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);
}

DebugModule::~DebugModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void DebugModule::propertySet() {
    m_widget->setInitialProperties({{"debugModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/debugModule.qml"));
}

void DebugModule::currentThreadSet(const QString &threadId) {
    if (m_currentThread == threadId) return;
    if (m_threads.isEmpty() ? !threadId.isEmpty() : !m_threads.contains(threadId)) return;
    m_currentThread = threadId;
    emit currentThreadChanged();
    emit callStackChanged();
}

void DebugModule::debugStart(const QString &threadId) {
    if (m_threads.contains(threadId)) return;
    m_threads.append(threadId);
    if (m_threads.size() == 1) currentThreadSet(threadId);
    emit threadsChanged();
}

void DebugModule::debugStop(const QString &threadId) {
    if (!m_threads.removeOne(threadId)) return;
    m_callStacks.remove(threadId);
    if (m_currentThread == threadId) currentThreadSet(m_threads.value(0));
    emit threadsChanged();
}

void DebugModule::stateSet(const Debug::Value state) {
    if (m_currentThread.isEmpty()) return;
    if (state == Debug::RunToCursor) emit getIndex();
    emit setState(m_currentThread, state);
}

void DebugModule::callStackSet(const QString &threadId, const QVariantList &frames) {
    if (!m_threads.contains(threadId)) return;
    m_callStacks.insert(threadId, frames);
    if (threadId == m_currentThread) emit callStackChanged();
}

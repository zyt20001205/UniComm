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
    debugRefresh();
}

void DebugModule::debugStart(const QString &threadId) {
    if (m_threads.contains(threadId)) return;
    m_threads.append(threadId);
    if (m_threads.size() == 1) currentThreadSet(threadId);
    emit threadsChanged();
}

void DebugModule::debugStop(const QString &threadId) {
    if (!m_threads.removeOne(threadId)) return;
    const bool wasPaused = m_callStacks.remove(threadId);
    if (m_currentThread == threadId) currentThreadSet(m_threads.value(0));
    else if (wasPaused) debugRefresh();
    emit threadsChanged();
}

void DebugModule::stateSet(const Debug::Value state) {
    if (m_currentThread.isEmpty()) return;
    if (state == Debug::RunToCursor) emit getIndex();
    emit setState(m_currentThread, state);
}

void DebugModule::callStackSet(const QString &threadId, const QVariantList &frames) {
    if (!m_threads.contains(threadId)) return;
    if (frames.isEmpty()) {
        m_callStacks.remove(threadId);
    } else {
        m_callStacks.insert(threadId, frames);
        const auto url = frames.constLast().toHash().value("documentUrl").toUrl();
        if (!url.isEmpty()) emit openDocument(url);
    }
    if (threadId == m_currentThread) emit callStackChanged();
    debugRefresh();
}

// private
void DebugModule::debugRefresh() {
    for (const auto &url: m_markedUrls) {
        emit clearEolAnnotation(url);
        emit deleteMarker(url, ScintillaMarker::Debug, -1);
        emit deleteMarker(url, ScintillaMarker::DebugLine, -1);
    }
    m_markedUrls.clear();
    QHash<QUrl, QHash<int, QStringList>> lines{};
    for (const auto &threadId: m_threads) {
        const auto stack = m_callStacks.constFind(threadId);
        if (stack == m_callStacks.cend()) continue;
        const auto frame = stack->constLast().toHash();
        const auto url = frame.value("documentUrl").toUrl();
        const int line = frame.value("line").toInt();
        if (url.isEmpty() || line < 0) continue;
        auto &threads = lines[url][line];
        if (threadId == m_currentThread) threads.prepend(threadId);
        else threads.append(threadId);
    }
    for (const auto &[url, fileLines]: lines.asKeyValueRange()) {
        m_markedUrls.insert(url);
        for (const auto &[line, threads]: fileLines.asKeyValueRange()) {
            emit addMarker(url, ScintillaMarker::Debug, line, -1);
            if (threads.first() == m_currentThread) {
                emit addMarker(url, ScintillaMarker::DebugLine, line, -1);
                emit setIndex(url, line, 0);
            }
            emit setEolAnnotation(url, line, threads.join(", "));
        }
    }
}

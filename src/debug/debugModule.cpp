#include "debug/debugModule.h"

#include <QQuickWidget>
#include <QStringList>

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
    m_currentThread = threadId;
    emit currentThreadChanged();
    emit callStackChanged();
    debugRefresh();
}

void DebugModule::interpreterStateSet(const InterpreterState::Value state) {
    if (m_currentThread.isEmpty()) return;
    if (state == InterpreterState::RunToCursor) emit getIndex();
    emit setInterpreterState(m_currentThread, state);
}

void DebugModule::callStackSet(const QString &threadId, const QVariantList &frames) {
    const bool paused = !frames.isEmpty();
    if (paused) {
        m_callStacks.insert(threadId, frames);
        const auto url = frames.constLast().toHash().value("documentUrl").toUrl();
        if (!url.isEmpty()) emit openDocument(url);
    } else {
        m_callStacks.remove(threadId);
    }
    emit setThreadStatus(threadId, paused ? ThreadStatus::Paused : ThreadStatus::Running);
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
    QHash<QUrl, QHash<int, QStringList> > lines{};
    for (const auto &[threadId, stack]: m_callStacks.asKeyValueRange()) {
        const auto frame = stack.constLast().toHash();
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

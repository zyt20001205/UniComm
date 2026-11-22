#include "debug/debugModule.h"

#include <QPromise>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QStringList>

#include "globals.h"
#include "core/globalManager.h"
#include "runtime/threadpoolModule.h"

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

QJsonArray DebugModule::threadsGet() const {
    QJsonArray result{};
    const auto threads = g_globalManager->threadpoolModuleGet()->threadsGet();
    for (const auto &[threadId, entry]: threads.asKeyValueRange()) {
        const auto thread = entry.toHash();
        if (thread.value("mode").value<RunMode::Value>() != RunMode::Debug) continue;
        const auto status = thread.value("status").value<ThreadStatus::Value>();
        result.append(threadInfo(threadId, status, m_callStacks.value(threadId)));
    }
    return result;
}

void DebugModule::currentThreadSet(const QString &threadId) {
    if (m_currentThread == threadId) return;
    m_currentThread = threadId;
    emit currentThreadChanged();
    emit callStackChanged();
    debugRefresh();
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

QFuture<QJsonObject> DebugModule::debugWait(const QString &threadId) {
    auto *pool = g_globalManager->threadpoolModuleGet();
    const auto thread = pool->threadsGet().value(threadId).toHash();
    if (thread.value("mode").value<RunMode::Value>() != RunMode::Debug)
        return QtFuture::makeReadyValueFuture(QJsonObject{{"error", "Expected an active debug thread."}});
    if (m_waitingThreads.contains(threadId))
        return QtFuture::makeReadyValueFuture(QJsonObject{{"error", "Already waiting for this thread."}});
    m_waitingThreads.insert(threadId);
    auto promise = QSharedPointer<QPromise<QJsonObject> >::create();
    promise->start();
    auto *request = new QObject(this);
    const auto finish = [this, threadId, pool, request, promise](const QJsonObject &result) {
        disconnect(pool, nullptr, request, nullptr);
        request->deleteLater();
        m_waitingThreads.remove(threadId);
        promise->addResult(result);
        promise->finish();
    };
    connect(pool, &ThreadpoolModule::setCallStack, request, [pool, threadId, finish](const QString &id, const QVariantList &frames) {
        if (id != threadId || frames.isEmpty()) return;
        const auto status = pool->threadsGet().value(id).toHash().value("status").value<ThreadStatus::Value>();
        if (status == ThreadStatus::Paused) finish(threadInfo(id, status, frames));
    });
    connect(pool, &ThreadpoolModule::finishThread, request, [threadId, finish](const QString &id, const QJsonObject &output) {
        if (id == threadId) finish(QJsonObject{{"thread_id", id}, {"status", "Finished"}, {"output", output}});
    });
    return promise->future();
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

QJsonObject DebugModule::threadInfo(const QString &threadId, const ThreadStatus::Value status, const QVariantList &frames) {
    QJsonObject row{{"thread_id", threadId}, {"status", QString::fromLatin1(QMetaEnum::fromType<ThreadStatus::Value>().valueToKey(status))}};
    if (status == ThreadStatus::Paused && !frames.isEmpty()) {
        const auto frame = frames.constLast().toHash();
        const auto url = frame.value("documentUrl").toUrl();
        const int line = frame.value("line").toInt();
        if (!url.isEmpty() && line >= 0) {
            row.insert("document_url", url.toString());
            row.insert("line", line);
        }
    }
    return row;
}

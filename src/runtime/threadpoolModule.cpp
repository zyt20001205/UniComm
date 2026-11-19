#include "runtime/threadpoolModule.h"

#include <QDateTime>
#include <QEventLoop>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QThread>

#include "globals.h"
#include "core/globalManager.h"
#include "runtime/luaInterpreter.h"
#include "document/documentModule.h"
#include "debug/watchModule.h"
#include "terminal/terminalPage.h"

// public
ThreadpoolModule::ThreadpoolModule()
    : DockWidget("Threadpool"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);
}

ThreadpoolModule::~ThreadpoolModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void ThreadpoolModule::propertySet() {
    m_widget->setInitialProperties({{"threadpoolModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/runtime/threadpoolModule.qml"));
}

void ThreadpoolModule::quit() {
    const auto total = m_threads.size();
    if (total == 0) return;
    QEventLoop loop{};
    connect(this, &ThreadpoolModule::finishThread, &loop, [this, total, &loop] {
        const auto current = total - m_threads.size();
        emit trackQuit(static_cast<float>(current) / static_cast<float>(total),
                       tr("Waiting for %1/%2 threads to terminate...").arg(current).arg(total));
        if (m_threads.isEmpty()) {
            emit trackQuit(0, "");
            loop.quit();
        }
    });
    emit trackQuit(0, tr("Waiting for 0/%1 threads to terminate...").arg(total));
    for (const auto &id: m_threads.keys()) threadStatusSet(id, ThreadStatus::Terminating);
    if (!m_threads.isEmpty()) loop.exec();
}

QString ThreadpoolModule::threadStart(const QUrl &documentUrl, const RunMode::Value mode, void *closure) {
    if (g_terminating) return {};
    std::variant<QString, LuaClosure> entry{};
    if (closure) entry = std::move(*static_cast<LuaClosure *>(closure));
    else entry = g_globalManager->documentModuleGet()->textGet(documentUrl);
    auto *worker = new QThread(); // NOLINT
    const auto threadId = QString("0x%1").arg(reinterpret_cast<quintptr>(worker), 0, 16);
    auto output = QSharedPointer<QJsonObject>::create();
    // preload thread with lua session
    QVariantMap luaSession{};
    luaSession.insert("mode", QVariant::fromValue(mode));
    luaSession.insert("workspaceUrl", g_workspaceUrl);
    luaSession.insert("documentUrl", documentUrl);
    luaSession.insert("threadId", threadId);
    if (mode == RunMode::Debug) {
        luaSession.insert("state", QVariant::fromValue(InterpreterState::Resume));
        luaSession.insert("baseDepth", 0);
        luaSession.insert("currentDepth", 0);
        luaSession.insert("expressions", QVariant::fromValue(g_globalManager->watchModuleGet()->watchExpressionsGet()));
    }
    auto *interpreter = new LuaInterpreter(luaSession); // NOLINT
    interpreter->moveToThread(worker);

    connect(interpreter, &LuaInterpreter::setCallStack, this, &ThreadpoolModule::setCallStack);
    connect(interpreter, &LuaInterpreter::setWatchValues, g_globalManager->watchModuleGet(), &WatchModule::watchValuesSet);
    connect(interpreter, &LuaInterpreter::writeTerminal, this, &ThreadpoolModule::writeTerminal);
    connect(interpreter, &LuaInterpreter::appendLog, this, &ThreadpoolModule::appendLog);
    connect(worker, &QThread::started, [this, threadId, worker, output, interpreter, entry = std::move(entry)] {
        *output = interpreter->start(entry);
        QMetaObject::invokeMethod(this, [this, threadId, worker] {
            get<QVariantHash>(m_threads[threadId]).remove("interpreter");
            emit threadsChanged();
            worker->quit();
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, interpreter, &LuaInterpreter::deleteLater);
    connect(worker, &QThread::finished, this, [this, threadId, worker, output] {
        m_threads.remove(threadId);
        emit threadsChanged();
        emit finishThread(threadId, *output);
        worker->deleteLater();
    });

    m_threads.insert(threadId, QVariantHash{
                         {"mode", QVariant::fromValue(mode)},
                         {"status", QVariant::fromValue(ThreadStatus::Running)},
                         {"name", documentUrl.fileName()},
                         {"createdAt", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")},
                         {"interpreter", QVariant::fromValue(QPointer(interpreter))}
                     });
    emit threadsChanged();
    emit openTerminal(threadId, TerminalPage::Backend::Lua);
    worker->start();
    return threadId;
}

void ThreadpoolModule::threadStatusSet(const QString &threadId, const ThreadStatus::Value status) {
    const auto entry = m_threads.find(threadId);
    if (entry == m_threads.end()) return;
    auto &thread = get<QVariantHash>(entry.value());
    if (thread.value("status").value<ThreadStatus::Value>() == ThreadStatus::Terminating) return;
    thread.insert("status", QVariant::fromValue(status));
    emit threadsChanged();
    if (status != ThreadStatus::Terminating) return;
    if (auto *interpreter = interpreterGet(threadId)) {
        interpreter->thread()->requestInterruption();
        QMetaObject::invokeMethod(interpreter, &LuaInterpreter::stop, Qt::QueuedConnection);
    }
}

void ThreadpoolModule::inputWrite(const QString &threadId, const QByteArray &data) const {
    if (const auto *interpreter = interpreterGet(threadId)) interpreter->inputWrite(data);
}

void ThreadpoolModule::interpreterStateSet(const QString &threadId, const InterpreterState::Value state) const {
    if (auto *interpreter = interpreterGet(threadId)) QMetaObject::invokeMethod(interpreter, &LuaInterpreter::interpreterStateSet, Qt::QueuedConnection, state);
}

void ThreadpoolModule::watchExpressionsSet(const QMultiHash<QUrl, QString> &expressions) const {
    for (const auto &[threadId, entry]: m_threads.asKeyValueRange()) {
        if (entry.toHash().value("mode").value<RunMode::Value>() != RunMode::Debug) continue;
        if (auto *interpreter = interpreterGet(threadId)) QMetaObject::invokeMethod(interpreter, &LuaInterpreter::watchExpressionsSet, Qt::QueuedConnection, expressions);
    }
}

void ThreadpoolModule::watchValuesGet(const QString &threadId) const {
    if (auto *interpreter = interpreterGet(threadId)) QMetaObject::invokeMethod(interpreter, &LuaInterpreter::watchValuesRefresh, Qt::QueuedConnection);
}

void ThreadpoolModule::watchValueSet(const QString &threadId, const QString &expression, const QString &value, const QString &type) const {
    if (auto *interpreter = interpreterGet(threadId)) emit interpreter->setWatchValue(expression, value, type);
}

// private
LuaInterpreter *ThreadpoolModule::interpreterGet(const QString &threadId) const {
    return m_threads.value(threadId).toHash().value("interpreter").value<QPointer<LuaInterpreter> >().data();
}

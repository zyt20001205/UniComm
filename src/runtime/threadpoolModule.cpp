#include "runtime/threadpoolModule.h"

#include <QDir>
#include <QHeaderView>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QThread>
#include <QTimer>
#include <QUuid>

#include "globals.h"
#include "core/globalManager.h"
#include "runtime/luaInterpreter.h"
#include "document/documentModule.h"
#include "terminal/terminalPage.h"

// public
ThreadpoolModule::ThreadpoolModule()
    : DockWidget("Threadpool"),
      m_widget(new QQuickWidget()),
      m_standardItemModel(new ThreadpoolModel(this)) {
    setWidget(m_widget);
}

ThreadpoolModule::~ThreadpoolModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void ThreadpoolModule::propertySet(const QVariantHash &objects) {
    m_mainItem = qvariant_cast<QObject *>(objects["mainItem"]);
    m_errorDialog = qvariant_cast<QObject *>(objects["threadpoolModuleErrorDialog"]);
    const QVariantList horizontalHeader = {"", tr("Source"), tr("Spawn Time"), tr("Thread ID")};

    m_widget->rootContext()->setContextProperty("threadpoolModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("threadMenu", qvariant_cast<QObject *>(objects["threadpoolModuleThreadMenu"]));
    m_widget->rootContext()->setContextProperty("horizontalHeader", horizontalHeader);
    m_widget->rootContext()->setContextProperty("standardItemModel", m_standardItemModel);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/runtime/threadpoolModule.qml"));
}

void ThreadpoolModule::quit() {
    if (const int total = static_cast<int>(m_threadHash.size())) {
        int current = 0;
        emit trackQuit(0, QString(tr("Waiting for 0/%1 threads to terminate...")).arg(total));

        QEventLoop eventLoop{};
        for (const auto &thread: m_threadHash) {
            connect(thread, &QThread::finished, this, [this, &current, &total, &eventLoop] {
                current++;
                emit trackQuit(static_cast<float>(current) / static_cast<float>(total),
                               QString(tr("Waiting for %1/%2 threads to terminate...")).arg(QString::number(current), QString::number(total)));
                if (current == total) {
                    emit trackQuit(0, "");
                    eventLoop.quit();
                }
            });
        }
        for (const auto &threadId: m_threadHash.keys()) {
            threadStop(threadId);
        }
        eventLoop.exec();
    }
}

void ThreadpoolModule::threadStart(const QUrl &documentUrl, const int mode, QString &threadId, const int startLine, const int startCharacter, const int endLine,
                                   const int endCharacter) {
    auto *worker = new QThread(); // NOLINT
    threadId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto output = QSharedPointer<QJsonObject>::create();
    // preload thread with lua session
    QVariantMap luaSession{};
    luaSession.insert("mode", mode);
    luaSession.insert("workspaceUrl", g_workspaceUrl);
    luaSession.insert("documentUrl", documentUrl);
    luaSession.insert("threadId", threadId);
    if (mode == InterpreterMode::Debug) {
        luaSession.insert("currentUrl", documentUrl);
        luaSession.insert("state", Debug::Resume);
        luaSession.insert("baseDepth", 0);
        luaSession.insert("currentDepth", 0);
    }
    auto *interpreter = new LuaInterpreter(luaSession); // NOLINT
    connect(interpreter, &LuaInterpreter::openDocument, this, &ThreadpoolModule::openDocument);
    connect(interpreter, &LuaInterpreter::addMarker, this, &ThreadpoolModule::addMarker);
    connect(interpreter, &LuaInterpreter::deleteMarker, this, &ThreadpoolModule::deleteMarker);
    connect(interpreter, &LuaInterpreter::insertCallStack, this, &ThreadpoolModule::insertCallStack);
    connect(interpreter, &LuaInterpreter::startThread, this,
            qOverload<const QUrl &, const int, QString &, const int, const int, const int, const int>(&ThreadpoolModule::threadStart), Qt::BlockingQueuedConnection);
    connect(interpreter, &LuaInterpreter::stopThread, this, &ThreadpoolModule::threadStop);
    connect(interpreter, &LuaInterpreter::writeTerminal, this, &ThreadpoolModule::writeTerminal);
    connect(interpreter, &LuaInterpreter::appendLog, this, &ThreadpoolModule::appendLog);
    connect(interpreter, &LuaInterpreter::newMessageDialog, this, &ThreadpoolModule::messageDialogNew);
    interpreter->moveToThread(worker);
    connect(worker, &QThread::finished, interpreter, &LuaInterpreter::deleteLater);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    // load thread with script
    const auto script = g_document->textGet(documentUrl, startLine, startCharacter, endLine, endCharacter);
    connect(worker, &QThread::started, [interpreter, script, output] {
        *output = interpreter->start(script);
        QThread::currentThread()->quit();
    });
    m_threadHash.insert(threadId, worker);
    m_interpreterHash.insert(threadId, interpreter);
    connect(worker, &QThread::finished, this, [this, threadId] { m_threadHash.remove(threadId); });
    connect(worker, &QThread::finished, this, [this, threadId] { m_interpreterHash.remove(threadId); });

    threadAppend(mode, documentUrl.fileName(), threadId);
    if (mode == InterpreterMode::Debug) {
        emit startDebug(threadId);
        connect(worker, &QThread::finished, this, [this, threadId] { emit stopDebug(threadId); });
    } else if (mode == InterpreterMode::Agent) {
        if (!m_agentTerminalId.isEmpty()) emit closeTerminal(m_agentTerminalId);
        m_agentTerminalId = threadId;
        connect(worker, &QThread::finished, this, [this, threadId, output] {
            if (output->value("output").toString().isEmpty() && output->value("err").toString().isEmpty()) emit closeTerminal(threadId);
            emit finishThread(threadId, *output);
        });
    }
    emit openTerminal(threadId, TerminalPage::Backend::Lua);
    worker->start();
}

void ThreadpoolModule::threadStart(const QUrl &documentUrl, const int mode, const int startLine, const int startCharacter, const int endLine, const int endCharacter) {
    QString threadId{};
    threadStart(documentUrl, mode, threadId, startLine, startCharacter, endLine, endCharacter);
}

void ThreadpoolModule::threadStop(const QString &threadId) {
    if (m_threadHash.contains(threadId)) {
        if (m_threadHash[threadId]->isInterruptionRequested()) {
            QMetaObject::invokeMethod(m_errorDialog, "open");
            return;
        }
        const auto indexes = m_standardItemModel->match(
            m_standardItemModel->index(0, THREADID_COL), Qt::DisplayRole, threadId, 1, Qt::MatchExactly);
        if (!indexes.isEmpty()) {
            const auto row = indexes.constFirst().row();
            const auto iconItem = m_standardItemModel->item(row, ICON_COL);
            iconItem->setData(InterpreterMode::Terminate, ThreadpoolModel::StatusRole);
            emit m_standardItemModel->dataChanged(
                m_standardItemModel->index(row, 0),
                m_standardItemModel->index(row, m_standardItemModel->columnCount() - 1),
                {ThreadpoolModel::StatusRole}
            );
            if (iconItem->data(ThreadpoolModel::ModeRole).toInt() == InterpreterMode::Debug) {
                stateSet(threadId, Debug::Terminate);
            }
        }
        m_threadHash[threadId]->requestInterruption();
    }
}

bool ThreadpoolModule::debugging() const {
    return m_debug != 0;
}

void ThreadpoolModule::inputWrite(const QString &threadId, const QByteArray &data) const {
    if (auto *interpreter = m_interpreterHash.value(threadId)) interpreter->inputWrite(data);
}

void ThreadpoolModule::stateSet(const QString &threadId, const int state) {
    if (m_interpreterHash.contains(threadId)) {
        if (state == Debug::Terminate) {
            m_threadHash[threadId]->requestInterruption();
        }
        auto *interpreter = m_interpreterHash[threadId];
        QMetaObject::invokeMethod(interpreter, [interpreter, state] { interpreter->stateSet(state); }, Qt::BlockingQueuedConnection);
    }
}

void ThreadpoolModule::valueSet(const QString &threadId, const QString &documentUrl, const QString &expression, const QString &value, const QString &type) {
    if (m_interpreterHash.contains(threadId)) {
        auto *interpreter = m_interpreterHash[threadId];
        QMetaObject::invokeMethod(interpreter, [interpreter, documentUrl, expression, value, type] {
            interpreter->valueSet(documentUrl, expression, value, type);
        }, Qt::BlockingQueuedConnection);
    }
}

// private
void ThreadpoolModule::threadAppend(const int mode, const QString &name, const QString &threadId) {
    const auto currentTime = QDateTime::currentDateTime();
    auto *iconItem = new QStandardItem(); // NOLINT
    iconItem->setData(mode, ThreadpoolModel::ModeRole);
    iconItem->setData(mode, ThreadpoolModel::StatusRole);
    iconItem->setData(threadId, ThreadpoolModel::ThreadIdRole);
    auto *nameItem = new QStandardItem(name); // NOLINT
    auto *spawnItem = new QStandardItem(currentTime.toString("yyyy-MM-dd HH:mm:ss.zzz")); // NOLINT
    auto *threadIdItem = new QStandardItem(threadId); // NOLINT
    m_standardItemModel->appendRow({iconItem, nameItem, spawnItem, threadIdItem});
    // refresh status bar
    if (mode == InterpreterMode::Run) m_run++;
    else if (mode == InterpreterMode::Debug) m_debug++;
    else if (mode == InterpreterMode::Agent) m_agent++;
    emit refreshThread(m_run, m_debug, m_agent);

    const auto *worker = m_threadHash[threadId];
    connect(worker, &QThread::finished, this, [this, threadId] {
        const auto indexes = m_standardItemModel->match(m_standardItemModel->index(0, THREADID_COL), Qt::DisplayRole, threadId, 1, Qt::MatchExactly);
        if (indexes.isEmpty()) return;
        const auto row = indexes.constFirst().row();
        const int mode = m_standardItemModel->item(row, ICON_COL)->data(ThreadpoolModel::ModeRole).toInt();
        m_standardItemModel->removeRow(row);
        // refresh status bar
        if (mode == InterpreterMode::Run) m_run--;
        else if (mode == InterpreterMode::Debug) m_debug--;
        else if (mode == InterpreterMode::Agent) m_agent--;
        emit refreshThread(m_run, m_debug, m_agent);
    });
}

void ThreadpoolModule::messageDialogNew(const QEventLoop *eventloop, const QString &threadId, const QString &text) const {
    QMetaObject::invokeMethod(m_mainItem, "messageDialogNew", Q_ARG(QVariant, QVariant::fromValue(eventloop)), Q_ARG(QVariant, threadId), Q_ARG(QVariant, text));
}

// public
ThreadpoolModel::ThreadpoolModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &ThreadpoolModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ThreadpoolModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &ThreadpoolModel::emptyChanged);
}

QHash<int, QByteArray> ThreadpoolModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[StatusRole] = "status";
    roles[ThreadIdRole] = "threadId";
    return roles;
}

QVariant ThreadpoolModel::data(const QModelIndex &index, const int role) const {
    if (role == StatusRole || role == ThreadIdRole) {
        return QStandardItemModel::data(this->index(index.row(), 0), role);
    }
    return QStandardItemModel::data(index, role);
}

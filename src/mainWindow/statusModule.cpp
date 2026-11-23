#include "mainWindow/statusModule.h"

#include <QDir>

#include "globals.h"
#include "core/globalManager.h"

// public
StatusModule::StatusModule(QWidget *parent)
    : QQuickWidget(g_globalManager->engineGet(), parent) {
}

StatusModule::~StatusModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] status module destructed").arg(timestamp);
}

void StatusModule::propertySet() {
    setInitialProperties({{"statusModule", QVariant::fromValue(this)}, {"workspaceName", g_workspaceUrl.fileName()}});
    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/mainWindow/statusModule.qml"));
}

QStringList StatusModule::documentPathGet(const QUrl &documentUrl) const {
    if (!documentUrl.isLocalFile()) return {};
    return QDir(g_workspaceUrl.toLocalFile()).relativeFilePath(documentUrl.toLocalFile()).split('/');
}

void StatusModule::backgroundAppend(int &taskId, const std::function<void()> &abort, const std::function<void()> &info) {
    taskId = m_taskId++;
    if (!abort && !info) return;
    if (abort) m_abortCallbacks.insert(taskId, abort);
    if (info) m_infoCallbacks.insert(taskId, info);
    m_tasks.append(QVariantHash{{"taskId", taskId}, {"message", QString{}}});
    emit tasksChanged();
}

void StatusModule::backgroundRemove(const int taskId) {
    const auto index = taskIndexGet(taskId);
    if (index < 0) return;
    m_tasks.removeAt(index);
    m_abortCallbacks.remove(taskId);
    m_infoCallbacks.remove(taskId);
    emit tasksChanged();
}

void StatusModule::backgroundRefresh(const int taskId, const QString &message) {
    const auto index = taskIndexGet(taskId);
    if (index < 0) return;
    auto &task = get<QVariantHash>(m_tasks[index]);
    if (task.value("message").toString() == message) return;
    task.insert("message", message);
    emit tasksChanged();
}

void StatusModule::backgroundAbort(const int taskId) {
    const auto callback = m_abortCallbacks.take(taskId);
    backgroundRemove(taskId);
    if (callback) callback();
}

void StatusModule::backgroundInfo(const int taskId) const {
    const auto callback = m_infoCallbacks.value(taskId);
    if (callback) callback();
}

// private
qsizetype StatusModule::taskIndexGet(const int taskId) const {
    for (qsizetype i = 0; i < m_tasks.size(); ++i)
        if (get<QVariantHash>(m_tasks.at(i)).value("taskId").toInt() == taskId) return i;
    return -1;
}

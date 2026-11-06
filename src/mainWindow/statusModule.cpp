#include "mainWindow/statusModule.h"

#include <QDir>
#include <QQmlContext>
#include <QQuickItem>
#include <Scintilla.h>

#include "globals.h"
#include "core/globalManager.h"

// public
StatusModule::StatusModule(QWidget *parent)
    : QQuickWidget(parent),
      m_backgroundModel(new BackgroundModel(this)) {
}

StatusModule::~StatusModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] status module destructed").arg(timestamp);
}

void StatusModule::propertySet(const QVariantHash &objects) {
    m_threadTooltip = qvariant_cast<QObject *>(objects["statusModuleThreadToolTip"]);
    auto *backgroundTooltip = qvariant_cast<QObject *>(objects["statusModuleBackgroundToolTip"]);
    backgroundTooltip->setProperty("backgroundModel", QVariant::fromValue<QObject *>(m_backgroundModel));
    rootContext()->setContextProperty("global", g_globalManager);

    rootContext()->setContextProperty("statusModule", this);
    rootContext()->setContextProperty("workspaceName", g_workspaceUrl.fileName());
    rootContext()->setContextProperty("eolModeMenu", qvariant_cast<QObject *>(objects["statusModuleEolModeMenu"]));
    rootContext()->setContextProperty("threadTooltip", m_threadTooltip);
    rootContext()->setContextProperty("backgroundTooltip", backgroundTooltip);
    rootContext()->setContextProperty("backgroundModel", m_backgroundModel);
    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/mainWindow/statusModule.qml"));
    m_root = rootObject();
}

void StatusModule::propertyGet(const QVariantMap &objects) {
    m_positionButton = qvariant_cast<QObject *>(objects["positionButton"]);
    m_eolModeButton = qvariant_cast<QObject *>(objects["eolModeButton"]);
    m_codePageButton = qvariant_cast<QObject *>(objects["codePageButton"]);
}

void StatusModule::backgroundAppend(int &taskId, const std::function<void()> &abort, const std::function<void()> &info) {
    taskId = m_taskId++;
    if (abort || info) {
        auto *item = new QStandardItem(); // NOLINT
        item->setData(taskId, BackgroundModel::TaskIdRole);
        m_backgroundModel->appendRow(item);
        backgroundUpdate();
        if (abort) m_abortCallbacks.insert(taskId, abort);
        if (info) m_infoCallbacks.insert(taskId, info);
    }
}

void StatusModule::backgroundRemove(const int taskId) {
    const auto indexes = m_backgroundModel->match(m_backgroundModel->index(0, 0), BackgroundModel::TaskIdRole, taskId, 1, Qt::MatchExactly);
    if (indexes.isEmpty()) return;
    m_backgroundModel->removeRow(indexes.constFirst().row());
    m_abortCallbacks.remove(taskId);
    m_infoCallbacks.remove(taskId);
    backgroundUpdate();
}

void StatusModule::backgroundRefresh(const int taskId, const QString &message) const {
    const auto indexes = m_backgroundModel->match(m_backgroundModel->index(0, 0), BackgroundModel::TaskIdRole, taskId, 1, Qt::MatchExactly);
    if (indexes.isEmpty()) return;
    m_backgroundModel->itemFromIndex(indexes.constFirst())->setText(message);
    backgroundUpdate();
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

void StatusModule::documentGoto(const QUrl &documentUrl) {
    emit gotoDocument(documentUrl);
}

void StatusModule::documentFocus(const QUrl &documentUrl, const QVariantHash &session) const {
    rootContext()->setContextProperty("documentUrl", documentUrl);
    const QString documentPath = documentUrl.toLocalFile();
    const QString workspacePath = g_workspaceUrl.toLocalFile();
    const QDir workspaceDir(workspacePath);
    const QString relativePath = workspaceDir.relativeFilePath(documentPath);
    const QVariantList pathList = QVariant::fromValue(relativePath.split('/')).toList();
    QMetaObject::invokeMethod(m_root, "documentPathLoad", Q_ARG(QVariant, QVariant::fromValue(pathList)));

    switch (session["eolMode"].toInt()) {
        case SC_EOL_CRLF: m_eolModeButton->setProperty("text", tr("CRLF"));
            break;
        case SC_EOL_CR: m_eolModeButton->setProperty("text", tr("CR"));
            break;
        case SC_EOL_LF: m_eolModeButton->setProperty("text", tr("LF"));
            break;
        default: break;
    }

    switch (session["codePage"].toInt()) {
        case 65001: m_codePageButton->setProperty("text", "UTF-8");
            break;
        case 932: m_codePageButton->setProperty("text", "Shift-JIS");
            break;
        case 936: m_codePageButton->setProperty("text", "GBK");
            break;
        case 949: m_codePageButton->setProperty("text", "EUC-KR");
            break;
        case 950: m_codePageButton->setProperty("text", "Big5");
            break;
        case 1361: m_codePageButton->setProperty("text", "JOHAB");
            break;
        default: break;
    }
}

void StatusModule::selectionChange(const QHash<QString, int> &selection) const {
    const auto current = QString("%1:%2").arg(QString::number(selection["line"] + 1), QString::number(selection["character"]));
    const auto charsText = selection["characters"] == 0 ? "" : QString(" [%1 chars]").arg(QString::number(selection["characters"]));
    const auto linesText = selection["lines"] == 0 ? "" : QString(" [%1 lines]").arg(QString::number(selection["lines"]));
    m_positionButton->setProperty("text", QString("%1%2%3").arg(current, charsText, linesText));
}

void StatusModule::threadRefresh(const int run, const int debug, const int agent) const {
    m_threadTooltip->setProperty("runCount", run);
    m_threadTooltip->setProperty("debugCount", debug);
    m_threadTooltip->setProperty("agentCount", agent);
}

// private
void StatusModule::backgroundUpdate() const {
    const auto taskCount = m_backgroundModel->rowCount();
    if (taskCount == 1) {
        const auto item = m_backgroundModel->item(0, 0);
        m_backgroundModel->titleSet(item->text());
        m_backgroundModel->taskIdSet(item->data(BackgroundModel::TaskIdRole).toInt());
    } else {
        m_backgroundModel->titleSet(tr("%1 tasks running.").arg(QString::number(taskCount)));
        m_backgroundModel->taskIdSet(-1);
    }
}

// public
BackgroundModel::BackgroundModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &BackgroundModel::taskCountChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &BackgroundModel::taskCountChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &BackgroundModel::taskCountChanged);
}

QHash<int, QByteArray> BackgroundModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[TaskIdRole] = "taskId";
    return roles;
}

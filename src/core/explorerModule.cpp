#include "core/explorerModule.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"
#include "types/gitTypes.h"
#include "util/uniCast.h"

// public
ExplorerModule::ExplorerModule()
    : DockWidget("Explorer"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_fileSystemModel(new QFileSystemModel(this)),
      m_sortFilterProxyModel(new SortFilterProxyModel(&m_documentStatus)),
      m_process(new QProcess(this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);
    connect(m_process, &QProcess::finished, this, [this](const int exitcode) { processFinished(exitcode); });
}

ExplorerModule::~ExplorerModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void ExplorerModule::propertySet() const {
    const auto modelRootPath = g_workspaceUrl.toLocalFile();
    m_fileSystemModel->setRootPath(modelRootPath);
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    m_sortFilterProxyModel->setSourceModel(m_fileSystemModel);
    const QModelIndex modelRootIndex = m_sortFilterProxyModel->mapFromSource(m_fileSystemModel->index(modelRootPath));

    m_widget->setInitialProperties({
        {"modelRootIndex", QVariant::fromValue(modelRootIndex)},
        {"modelRootUrl", g_workspaceUrl},
        {"sortFilterProxyModel", QVariant::fromValue<QAbstractItemModel *>(m_sortFilterProxyModel)}
    });

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/core/explorerModule.qml"));
}

void ExplorerModule::indexUpdate() const {
    const auto &rootPath = g_globalManager->serviceModuleGet()->gitServiceGet()->rootPathGet();
    if (rootPath.isEmpty()) return;
    m_process->setWorkingDirectory(rootPath);
    m_process->start("git", {"--no-optional-locks", "status", "-uall", "--porcelain", "--ignored"});
}

void ExplorerModule::scriptRun(const QUrl &documentUrl) {
    emit startThread(documentUrl, RunMode::Run);
}

void ExplorerModule::scriptDebug(const QUrl &documentUrl) {
    emit startThread(documentUrl, RunMode::Debug);
}

void ExplorerModule::documentOpen(const QUrl &documentUrl) {
    emit openDocument(documentUrl);
}

bool ExplorerModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_widget && event->type() == QEvent::FocusOut) emit clearSelection();
    return DockWidget::eventFilter(watched, event);
}

void ExplorerModule::processFinished(const int exitcode) {
    const auto output = m_process->readAllStandardOutput();
    const auto lines = QString(output).split('\n', Qt::SkipEmptyParts);
    m_documentStatus.clear();
    for (const auto &line: lines) {
        const auto indexStatus = GitFileStatus::fromCode(line.at(0));
        const auto workingTreeStatus = GitFileStatus::fromCode(line.at(1));
        auto filePath = line.mid(3).trimmed();
        if (indexStatus == GitFileStatus::Renamed || indexStatus == GitFileStatus::Copied || workingTreeStatus == GitFileStatus::Renamed || workingTreeStatus ==
            GitFileStatus::Copied)
            filePath = filePath.section(" -> ", 1);
        const auto documentPath = QDir(m_process->workingDirectory()).filePath(filePath);
        const auto documentUrl = QUrl::fromLocalFile(documentPath);
        m_documentStatus[documentUrl] = QVariantHash{
            {"indexStatus", indexStatus},
            {"workingTreeStatus", workingTreeStatus}
        };
    }
    m_sortFilterProxyModel->invalidate();
}

SortFilterProxyModel::SortFilterProxyModel(const QHash<QUrl, QVariant> *documentStatus, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_documentStatus(documentStatus) {
}

QHash<int, QByteArray> SortFilterProxyModel::roleNames() const {
    auto roles = QSortFilterProxyModel::roleNames();
    roles[SourceRole] = "source";
    roles[IsDirRole] = "isDir";
    roles[DocumentUrlRole] = "documentUrl";
    roles[GitRole] = "git";
    return roles;
}

QVariant SortFilterProxyModel::data(const QModelIndex &index, const int role) const {
    const auto sourceIndex = mapToSource(index);
    const auto *fileModel = qobject_cast<QFileSystemModel *>(sourceModel());
    const auto fileInfo = fileModel->fileInfo(sourceIndex);
    const auto documentUrl = QUrl::fromLocalFile(fileInfo.filePath());
    if (role == SourceRole) {
        const auto &source = uni_cast<QFileIcon>(documentUrl);
        return source.value;
    }
    if (role == IsDirRole) {
        return fileInfo.isDir();
    }
    if (role == DocumentUrlRole) {
        return documentUrl;
    }
    if (role == GitRole) {
        if (g_globalManager->serviceModuleGet()->gitServiceGet()->rootPathGet().isEmpty()) return {};
        return m_documentStatus->value(documentUrl);
    }
    return QSortFilterProxyModel::data(index, role);
}

int SortFilterProxyModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 1;
}

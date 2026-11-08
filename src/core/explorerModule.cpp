#include "core/explorerModule.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QQmlContext>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "util/uniCast.h"

// public
ExplorerModule::ExplorerModule()
    : DockWidget("Explorer"),
      m_widget(new QQuickWidget()),
      m_fileSystemModel(new QFileSystemModel(this)),
      m_sortFilterProxyModel(new SortFilterProxyModel(&m_documentStatus)),
      m_process(new QProcess(this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);
    m_process->setWorkingDirectory(g_workspaceUrl.toLocalFile());
    connect(m_process, &QProcess::finished, this, [this](const int exitcode) { processFinished(exitcode); });
}

ExplorerModule::~ExplorerModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void ExplorerModule::propertySet(const QVariantHash &objects) {
    const auto modelRootPath = g_workspaceUrl.toLocalFile();
    m_fileSystemModel->setRootPath(modelRootPath);
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    m_sortFilterProxyModel->setSourceModel(m_fileSystemModel);
    const QModelIndex modelRootIndex = m_sortFilterProxyModel->mapFromSource(m_fileSystemModel->index(modelRootPath));

    m_widget->rootContext()->setContextProperty("explorerModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_widget->rootContext()->setContextProperty("fileMenu", objects["explorerModuleFileMenu"]);
    m_widget->rootContext()->setContextProperty("folderMenu", objects["explorerModuleFolderMenu"]);
    m_widget->rootContext()->setContextProperty("rootMenu", objects["explorerModuleRootMenu"]);
    m_widget->rootContext()->setContextProperty("modelRootIndex", modelRootIndex);
    m_widget->rootContext()->setContextProperty("modelRootPath", modelRootPath);
    m_widget->rootContext()->setContextProperty("modelRootUrl", g_workspaceUrl);
    m_widget->rootContext()->setContextProperty("sortFilterProxyModel", m_sortFilterProxyModel);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/core/explorerModule.qml"));
}

void ExplorerModule::propertyGet(const QVariantMap &objects) {
    m_treeView = qvariant_cast<QObject *>(objects["treeView"]);
}

void ExplorerModule::indexUpdate() const {
    m_process->start("git", {"--no-optional-locks", "status", "-uall", "--porcelain", "--ignored"});
}

void ExplorerModule::scriptRun(const QUrl &documentUrl) {
    emit startThread(documentUrl, InterpreterMode::Run);
}

void ExplorerModule::scriptDebug(const QUrl &documentUrl) {
    emit startThread(documentUrl, InterpreterMode::Debug);
}

void ExplorerModule::documentOpen(const QUrl &documentUrl) {
    emit openDocument(documentUrl);
}

bool ExplorerModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_widget) {
        if (event->type() == QEvent::FocusOut) {
            m_treeView->setProperty("selectedRow", -1);
        }
    }
    return DockWidget::eventFilter(watched, event);
}

void ExplorerModule::processFinished(const int exitcode) {
    const auto output = m_process->readAllStandardOutput();
    const auto lines = QString(output).split('\n', Qt::SkipEmptyParts);
    m_documentStatus.clear();
    for (const auto &line: lines) {
        const auto indexStatus = line.at(0);
        const auto workingTreeStatus = line.at(1);
        auto filePath = line.mid(3).trimmed();
        if (indexStatus == 'R' || indexStatus == 'C' || workingTreeStatus == 'R' || workingTreeStatus == 'C') filePath = filePath.section(" -> ", 1);
        const auto documentPath = QDir(g_gitRootPath).filePath(filePath);
        const auto documentUrl = QUrl::fromLocalFile(documentPath);
        m_documentStatus[documentUrl] = QVariantHash{
                    {"indexStatus", g_gitStatusCode[indexStatus]},
                    {"workingTreeStatus", g_gitStatusCode[workingTreeStatus]}
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
        if (!g_globalManager->gitEnabledGet() || !m_documentStatus->contains(documentUrl)) {
            return {};
        }
        const auto gitStatus = m_documentStatus->value(documentUrl).toHash();
        const auto indexStatus = gitStatus["indexStatus"].toInt();
        const auto workingTreeStatus = gitStatus["workingTreeStatus"].toInt();
        return QVariantHash{
            {"indexStatus", indexStatus},
            {"workingTreeStatus", workingTreeStatus}
        };
    }
    return QSortFilterProxyModel::data(index, role);
}

int SortFilterProxyModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 1;
}

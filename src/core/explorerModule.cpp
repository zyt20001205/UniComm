#include "core/explorerModule.h"

#include <QFileSystemModel>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/serviceModule.h"
#include "util/uniCast.h"

// public
ExplorerModule::ExplorerModule()
    : DockWidget("Explorer"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_fileSystemModel(new QFileSystemModel(this)),
      m_sortFilterProxyModel(new SortFilterProxyModel(&m_documentStatus)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);
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

void ExplorerModule::indexUpdate() {
    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    if (!git->hasRepositoryGet()) return;
    git->statusGet(true).then(this, [this](const GitStatusResult &result) {
        if (!result.success) return;
        m_documentStatus = result.files;
        m_sortFilterProxyModel->invalidate();
    });
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

SortFilterProxyModel::SortFilterProxyModel(const QHash<QUrl, GitStatusResult::Entry> *documentStatus, QObject *parent)
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
        const auto entry = m_documentStatus->constFind(documentUrl);
        if (entry == m_documentStatus->cend()) return {};
        return QVariantHash{{"indexStatus", entry->indexStatus}, {"workingTreeStatus", entry->workingTreeStatus}};
    }
    return QSortFilterProxyModel::data(index, role);
}

int SortFilterProxyModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 1;
}

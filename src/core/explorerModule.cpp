#include "core/explorerModule.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "service/serviceModule.h"
#include "util/uniCast.h"

// public
ExplorerModule::ExplorerModule()
    : DockWidget("Explorer"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_model(new ExplorerModel(this)) {
    setWidget(m_widget);
    m_widget->setAcceptDrops(true);
    m_widget->installEventFilter(this);
    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    connect(git, &GitService::indexChanged, this, &ExplorerModule::indexUpdate);
    connect(git, &GitService::hasRepoChanged, this, [this, git] {
        if (!git->hasRepoGet()) indexUpdate();
    });
    indexUpdate();
}

ExplorerModule::~ExplorerModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void ExplorerModule::propertySet() const {
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    const auto modelRootIndex = m_model->setRootPath(g_workspaceUrl.toLocalFile());

    m_widget->setInitialProperties({
        {"modelRootIndex", QVariant::fromValue(modelRootIndex)},
        {"modelRootUrl", g_workspaceUrl}
    });

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/core/explorerModule.qml"));
}

void ExplorerModule::indexUpdate() {
    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    if (!git->hasRepoGet()) {
        m_model->gitStatusSet({});
        return;
    }
    git->statusGet(true).then(this, [this](const GitStatusResult &result) {
        if (!result.success) return;
        m_model->gitStatusSet(result.files);
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

QHash<int, QByteArray> ExplorerModel::roleNames() const {
    auto roles = QFileSystemModel::roleNames();
    roles[SourceRole] = "source";
    roles[IsDirRole] = "isDir";
    roles[DocumentUrlRole] = "documentUrl";
    roles[GitRole] = "git";
    return roles;
}

QVariant ExplorerModel::data(const QModelIndex &index, const int role) const {
    switch (role) {
        case SourceRole: return uni_cast<QFileIcon>(QUrl::fromLocalFile(filePath(index))).value;
        case IsDirRole: return isDir(index);
        case DocumentUrlRole: return QUrl::fromLocalFile(filePath(index));
        case GitRole: {
            const auto entry = m_documentStatus.constFind(QUrl::fromLocalFile(filePath(index)));
            if (entry == m_documentStatus.cend()) return {};
            return QVariantHash{{"indexStatus", entry->indexStatus}, {"workingTreeStatus", entry->workingTreeStatus}};
        }
        default: return QFileSystemModel::data(index, role);
    }
}

int ExplorerModel::columnCount(const QModelIndex &parent) const {
    return parent.column() > 0 ? 0 : 1;
}

void ExplorerModel::gitStatusSet(const QHash<QUrl, GitStatusResult::Entry> &files) {
    m_documentStatus = files;
    gitStatusNotify(index(rootPath()));
}

void ExplorerModel::gitStatusNotify(const QModelIndex &parent) {
    const int rows = rowCount(parent);
    if (!rows) return;
    emit dataChanged(index(0, 0, parent), index(rows - 1, 0, parent), {GitRole});
    for (int row = 0; row < rows; ++row) gitStatusNotify(index(row, 0, parent));
}

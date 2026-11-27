#include "service/git/gitCommitModule.h"

#include <QDir>
#include <QFileInfo>
#include <QQuickWidget>

#include "core/globalManager.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"

// public
GitCommitModule::GitCommitModule()
    : DockWidget("Commit"),
      m_gitService(g_globalManager->serviceModuleGet()->gitServiceGet()),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_changes(new QStandardItemModel(this)) {
    setWidget(m_widget);
    m_changes->setItemRoleNames({
        {Qt::DisplayRole, "display"}, {DocumentUrlRole, "documentUrl"}, {DirectoryRole, "directory"},
        {FileStatusRole, "fileStatus"}, {FileCountRole, "fileCount"}
    });
    for (const auto &title: {tr("Changes"), tr("Unversioned Files")}) {
        auto *group = new QStandardItem(title);
        group->setColumnCount(1);
        group->setData(0, FileCountRole);
        m_changes->appendRow(group);
    }
}

void GitCommitModule::propertySet() {
    m_widget->setInitialProperties({{"gitCommitModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/service/git/gitCommitModule.qml"));

    connect(m_gitService, &GitService::indexChanged, this, &GitCommitModule::indexUpdate);
    indexUpdate();
}

void GitCommitModule::commitShow() {
    if (!isOpen()) open();
    raise();
}

// private
void GitCommitModule::indexUpdate() {
    if (!m_gitService->hasRepoGet()) return;
    m_gitService->statusGet().then(this, [this](const GitStatusResult &result) {
        if (!result.success) return;
        auto *changes = m_changes->item(0);
        auto *unversioned = m_changes->item(1);
        changes->removeRows(0, changes->rowCount());
        unversioned->removeRows(0, unversioned->rowCount());

        const QDir workTree(m_gitService->repoGet().workTreePath);
        for (const auto &file: result.files) {
            const QFileInfo info(file.documentUrl.toLocalFile());
            auto *item = new QStandardItem(info.fileName());
            item->setData(file.documentUrl, DocumentUrlRole);
            item->setData(workTree.relativeFilePath(info.absolutePath()), DirectoryRole);
            item->setData(QVariant::fromValue(file.fileStatus()), FileStatusRole);
            auto *group = file.indexStatus == GitFileStatus::Untracked ? unversioned : changes;
            group->appendRow(item);
        }
        for (auto *group: {changes, unversioned}) group->setData(group->rowCount(), FileCountRole);
        emit indexUpdated();
    });
}

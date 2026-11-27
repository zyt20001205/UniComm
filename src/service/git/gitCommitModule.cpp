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
        {Qt::DisplayRole, "display"}, {Qt::CheckStateRole, "checkState"}, {DocumentUrlRole, "documentUrl"}, {DirectoryRole, "directory"},
        {FileStatusRole, "fileStatus"}, {FileCountRole, "fileCount"}
    });
    for (const auto &title: {tr("Changes"), tr("Unversioned Files")}) {
        auto *group = new QStandardItem(title);
        group->setColumnCount(1);
        group->setCheckState(Qt::Unchecked);
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

void GitCommitModule::checkStateSet(const QModelIndex &index, const bool checked) const {
    auto *item = m_changes->itemFromIndex(index);
    if (!item) return;
    const auto state = checked ? Qt::Checked : Qt::Unchecked;
    item->setCheckState(state);
    if (auto *group = item->parent()) {
        groupCheckStateUpdate(group);
    } else {
        for (int row = 0; row < item->rowCount(); ++row) item->child(row)->setCheckState(state);
    }
}

// private
void GitCommitModule::indexUpdate() {
    if (!m_gitService->hasRepoGet()) return;
    m_gitService->statusGet().then(this, [this](const GitStatusResult &result) {
        if (!result.success) return;
        auto *changes = m_changes->item(0);
        auto *unversioned = m_changes->item(1);
        QHash<QUrl, bool> checked{};
        for (auto *group: {changes, unversioned}) {
            for (int row = 0; row < group->rowCount(); ++row) {
                const auto *item = group->child(row);
                checked.insert(item->data(DocumentUrlRole).toUrl(), item->checkState() == Qt::Checked);
            }
            group->removeRows(0, group->rowCount());
        }

        const QDir workTree(m_gitService->repoGet().workTreePath);
        for (const auto &file: result.files) {
            const QFileInfo info(file.documentUrl.toLocalFile());
            auto *item = new QStandardItem(GitFileStatus::displayName(file.documentUrl, file.originalUrl, file.similarity));
            item->setData(file.documentUrl, DocumentUrlRole);
            item->setData(workTree.relativeFilePath(info.absolutePath()), DirectoryRole);
            item->setData(QVariant::fromValue(file.fileStatus()), FileStatusRole);
            auto *group = file.indexStatus == GitFileStatus::Untracked ? unversioned : changes;
            item->setCheckState(checked.value(file.documentUrl) ? Qt::Checked : Qt::Unchecked);
            group->appendRow(item);
        }
        for (auto *group: {changes, unversioned}) {
            group->setData(group->rowCount(), FileCountRole);
            groupCheckStateUpdate(group);
        }
        emit indexUpdated();
    });
}

void GitCommitModule::groupCheckStateUpdate(QStandardItem *group) {
    int checked = 0;
    for (int row = 0; row < group->rowCount(); ++row) {
        if (group->child(row)->checkState() == Qt::Checked) ++checked;
    }
    if (checked == 0) group->setCheckState(Qt::Unchecked);
    else if (checked == group->rowCount()) group->setCheckState(Qt::Checked);
    else group->setCheckState(Qt::PartiallyChecked);
}

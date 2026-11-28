#include "service/git/gitCommitModule.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQuickWidget>

#include "core/globalManager.h"
#include "document/documentModule.h"
#include "mainWindow/toastModule.h"
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
        {Qt::DisplayRole, "display"}, {Qt::CheckStateRole, "checkState"}, {FileStatusRole, "fileStatus"},
        {DocumentUrlRole, "documentUrl"}, {OriginalUrlRole, "originalUrl"}, {DirectoryRole, "directory"}, {FileCountRole, "fileCount"}
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

void GitCommitModule::commit(const QString &message, const bool amend, const bool push) {
    QList<QUrl> selectedUrls{};
    QList<QUrl> untrackedUrls{};
    for (int groupRow = 0; groupRow < m_changes->rowCount(); ++groupRow) {
        const auto *group = m_changes->item(groupRow);
        for (int row = 0; row < group->rowCount(); ++row) {
            const auto *item = group->child(row);
            if (item->checkState() != Qt::Checked) continue;
            const auto url = item->data(DocumentUrlRole).toUrl();
            const auto status = item->data(FileStatusRole).value<GitFileStatus::Value>();
            selectedUrls.append(url);
            if (status == GitFileStatus::Renamed) selectedUrls.append(item->data(OriginalUrlRole).toUrl());
            if (status == GitFileStatus::Untracked) untrackedUrls.append(url);
        }
    }
    m_gitService->commit(selectedUrls, untrackedUrls, message, amend).then(this, [this, push](const GitResult &result) {
        if (result.success) emit committed(push);
        else g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Commit failed"), result.text());
    });
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

void GitCommitModule::rollback(const GitFileStatus::Value fileStatus, const QUrl &documentUrl, const QUrl &originalUrl) {
    if (documentUrl.isEmpty() || fileStatus == GitFileStatus::Untracked || fileStatus == GitFileStatus::Unmerged) return;

    const auto mode = fileStatus == GitFileStatus::Added || fileStatus == GitFileStatus::Copied ? GitRestoreMode::Staged : GitRestoreMode::Both;
    QList urls{documentUrl};
    if (fileStatus == GitFileStatus::Renamed) urls.append(originalUrl);
    m_gitService->restore(urls, mode).then(this, [](const GitResult &result) {
        if (!result.success) g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Rollback failed"), result.text());
    });
}

void GitCommitModule::diffShow(const GitFileStatus::Value fileStatus, const QUrl &documentUrl, const QUrl &originalUrl) {
    if (fileStatus == GitFileStatus::Unmerged) {
        g_globalManager->documentModuleGet()->documentOpen(documentUrl);
        return;
    }

    const auto leftUrl = originalUrl.isEmpty() ? documentUrl : originalUrl;
    const auto leftTitle = tr("%1 · HEAD").arg(leftUrl.fileName());
    const auto rightTitle = tr("%1 · Working Tree").arg(documentUrl.fileName());
    QByteArray rightContent{};
    QFile file(documentUrl.toLocalFile());
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) rightContent = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Compare failed"), file.errorString());
            return;
        }
    }
    if (fileStatus == GitFileStatus::Added || fileStatus == GitFileStatus::Untracked) {
        g_globalManager->documentModuleGet()->diffOpen(leftTitle, {}, rightTitle, rightContent);
        return;
    }
    const QDir workTree(m_gitService->repoGet().workTreePath);
    const auto relativePath = workTree.relativeFilePath(leftUrl.toLocalFile());
    m_gitService->blobGet("HEAD:" + relativePath).then(this, [leftTitle, rightTitle, rightContent](const GitResult &result) {
        if (!result.success) {
            g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Compare failed"), result.text());
            return;
        }
        g_globalManager->documentModuleGet()->diffOpen(leftTitle, result.raw, rightTitle, rightContent);
    });
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
            item->setData(QVariant::fromValue(file.fileStatus()), FileStatusRole);
            item->setData(file.documentUrl, DocumentUrlRole);
            item->setData(file.originalUrl, OriginalUrlRole);
            item->setData(workTree.relativeFilePath(info.absolutePath()), DirectoryRole);
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

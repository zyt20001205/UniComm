#include "service/git/gitModule.h"

#include <kddockwidgets/core/DockWidget.h>
#include <QDir>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWidget>

#include "core/globalManager.h"
#include "mainWindow/mainWindow.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"
#include "util/uniCast.h"

// public
GitModule::GitModule()
    : DockWidget("Git"),
      m_gitService(g_globalManager->serviceModuleGet()->gitServiceGet()),
      m_gitStatus(GitOperationState::Idle),
      m_config(g_workspaceConfig["gitConfig"].toObject()),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_commitWindow(new QQuickView(g_globalManager->engineGet(), nullptr)),
      m_pushWindow(new QQuickView(g_globalManager->engineGet(), nullptr)),
      m_process(new QProcess(this)),
      m_refs(new QStandardItemModel(this)),
      m_log(new QStandardItemModel(this)),
      m_show(new QStandardItemModel(this)),
      m_workingTreeModel(new StatusModel(this)),
      m_indexModel(new StatusModel(this)),
      m_commitModel(new CommitModel(this)),
      m_showModel_(new ShowModel(this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);

    m_refs->setItemRoleNames({{Qt::DisplayRole, "display"}, {RefRole, "ref"}, {TypeRole, "type"}});
    m_refs->setColumnCount(1);
    m_log->setItemRoleNames({{Qt::DisplayRole, "display"}, {HashRole, "hash"}, {PositionRole, "position"}, {ParentsRole, "parents"}});
    m_log->setColumnCount(4);
    m_show->setItemRoleNames({{Qt::DisplayRole, "display"}, {Qt::DecorationRole, "decoration"}, {StatusRole, "status"}});
    m_show->setColumnCount(1);

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_EDITOR", ":");
    m_process->setProcessEnvironment(env);
    m_process->setWorkingDirectory(g_workspaceUrl.toLocalFile());
    connect(m_process, &QProcess::finished, this, [this](const int exitcode) { processFinished(exitcode); });
}

GitModule::~GitModule() {
    delete m_commitWindow;
    delete m_pushWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void GitModule::propertySet() {
    m_widget->setInitialProperties({{"gitModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/service/git/gitModule.qml"));

    // commit window
    m_commitWindow->setTitle(tr("Git Commit"));
    m_commitWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());

    m_commitWindow->setInitialProperties({
        {"gitModule", QVariant::fromValue(this)},
        {"workingTreeModel", QVariant::fromValue(m_workingTreeModel)},
        {"indexModel", QVariant::fromValue(m_indexModel)}
    });
    m_commitWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_commitWindow->setSource(QUrl("qrc:/qml/service/git/gitCommitWindow.qml"));

    // push window
    m_pushWindow->setTitle(tr("Git Push"));
    m_pushWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());

    m_pushWindow->setInitialProperties({
        {"gitModule", QVariant::fromValue(this)},
        {"commitModel", QVariant::fromValue(m_commitModel)},
        {"showModel", QVariant::fromValue(m_showModel_)}
    });
    m_pushWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_pushWindow->setSource(QUrl("qrc:/qml/service/git/gitPushWindow.qml"));

    connect(m_gitService, &GitService::gitDirChanged, this, &GitModule::gitRepo);
    connect(m_gitService, &GitService::refsChanged, this, &GitModule::upstreamGet);
    connect(m_gitService, &GitService::indexChanged, this, &GitModule::statusGet);
    if (m_gitService->hasRepoGet()) {
        gitRepo();
        upstreamGet();
    }
}

void GitModule::saveState() const {
    QVariant state{};
    if (!QMetaObject::invokeMethod(m_widget->rootObject(), "saveState", qReturnArg(state))) return;
    auto data = dockWidget()->userData();
    data.insert("splitView", QString::fromLatin1(state.toByteArray().toBase64()));
    dockWidget()->setUserData(data);
}

void GitModule::restoreState() const {
    const auto encoded = dockWidget()->userData().value("splitView").toString();
    if (!encoded.isEmpty())
        QMetaObject::invokeMethod(m_widget->rootObject(), "restoreState", Q_ARG(QVariant, QByteArray::fromBase64(encoded.toLatin1())));
}

// public: operation
void GitModule::operationContinue(const QString &message) {
    m_gitService->operationContinue(m_gitStatus, message).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Continue Failed"), result.text());
    });
}

void GitModule::operationAbort() {
    m_gitService->operationAbort(m_gitStatus).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Abort Failed"), result.text());
    });
}

// public: repo
void GitModule::_init() {
    m_gitService->init().then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

void GitModule::gitRepo() {
    const auto &repo = m_gitService->repoGet();
    m_process->setWorkingDirectory(repo.workTreePath);
    if (m_gitStatus == GitOperationState::Transfer) return;
    const QDir gitDir(repo.gitDirPath);
    auto state = GitOperationState::Idle;
    if (gitDir.exists("REBASE_HEAD") && (gitDir.exists("rebase-merge") || gitDir.exists("rebase-apply/rebasing"))) state = GitOperationState::Rebase;
    else if (gitDir.exists("MERGE_HEAD")) state = GitOperationState::Merge;
    else if (gitDir.exists("CHERRY_PICK_HEAD")) state = GitOperationState::CherryPick;
    else if (gitDir.exists("REVERT_HEAD")) state = GitOperationState::Revert;
    if (state != m_gitStatus) {
        if (m_gitStatus != GitOperationState::Idle) emit removeBackground(m_taskId);
        if (state != GitOperationState::Idle)
            m_taskId = emit appendBackground([this] { operationAbort(); }, [this] { operationRefresh(); });
        gitStatusSet(state);
    }
    if (state != GitOperationState::Idle) operationRefresh();
}

// public: remote
void GitModule::fetch() {
    if (m_gitStatus != GitOperationState::Idle) return;
    gitStatusSet(GitOperationState::Transfer);
    auto future = m_gitService->fetch();
    const int taskId = emit appendBackground([this] { m_gitService->transferCancel(); }, {});
    emit refreshBackground(taskId, tr("Fetching from remote..."));
    future.then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Fetch Failed"), result.text());
    }).onCanceled(this, [] {
    }).then(this, [this, taskId] {
        emit removeBackground(taskId);
        gitStatusSet(GitOperationState::Idle);
    });
}

void GitModule::push() {
    if (m_gitStatus != GitOperationState::Idle) return;
    m_pushWindow->close();
    gitStatusSet(GitOperationState::Transfer);
    auto future = m_gitService->push();
    const int taskId = emit appendBackground([this] { m_gitService->transferCancel(); }, {});
    emit refreshBackground(taskId, tr("Pushing to remote..."));
    future.then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Push Failed"), result.text());
    }).onCanceled(this, [] {
    }).then(this, [this, taskId] {
        emit removeBackground(taskId);
        gitStatusSet(GitOperationState::Idle);
    });
}

// public: branch

// always -> refsGet()
void GitModule::upstreamGet() {
    m_gitService->upstreamGet().then(this, [this](const GitResult &result) {
        m_upstream = result.success ? result.text() : QString{};
        refsGet();
    });
}

// failure -> showErrorDialog()
void GitModule::upstreamSet(const QString &upstream) {
    m_gitService->upstreamSet(upstream).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// failure -> showErrorDialog()
void GitModule::upstreamUnset() {
    m_gitService->upstreamUnset().then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// success -> logGet()
// failure -> showErrorDialog()
void GitModule::refsGet() {
    m_gitService->refsGet().then(this, [this](const GitRefsResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.text());
            return;
        }

        m_refs->removeRows(0, m_refs->rowCount());
        m_current.clear();
        QHash<QString, QStandardItem *> groups{};
        for (const auto &[ref, current]: result.refs) {
            const auto kind = ref.section('/', 1, 1);
            const auto name = ref.section('/', 2);
            auto &group = groups[kind];
            if (!group) {
                group = new QStandardItem;
                if (kind == "heads") {
                    group->setText(tr("Local"));
                    group->setData(QVariant::fromValue(GitRefType::LocalGroup), TypeRole);
                } else if (kind == "remotes") {
                    group->setText(tr("Remote"));
                    group->setData(QVariant::fromValue(GitRefType::RemoteGroup), TypeRole);
                } else {
                    group->setText(tr("Tags"));
                    group->setData(QVariant::fromValue(GitRefType::TagGroup), TypeRole);
                }
                m_refs->appendRow(group);
            }

            auto *item = new QStandardItem(name);
            item->setData(ref, RefRole);
            if (kind == "heads") {
                if (current) m_current = name;
                item->setData(QVariant::fromValue(current ? GitRefType::Current : GitRefType::Local), TypeRole);
            } else if (kind == "remotes") {
                item->setData(QVariant::fromValue(ref == m_upstream ? GitRefType::Upstream : GitRefType::Remote), TypeRole);
            } else {
                item->setData(QVariant::fromValue(GitRefType::Tag), TypeRole);
            }
            if (ref == "refs/heads/master") group->insertRow(0, item);
            else group->appendRow(item);
        }
        emit currentChanged();
        if (!m_current.isEmpty()) logGet("refs/heads/" + m_current);
    });
}

// failure -> showErrorDialog()
void GitModule::branchCreate(const QString &startPoint, const QString &name, const bool checkout) {
    m_gitService->branchCreate(startPoint, name, checkout).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// failure -> showErrorDialog()
void GitModule::branchSwitch(const QString &name) {
    m_gitService->branchSwitch(name).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// failure -> showErrorDialog()
void GitModule::branchRename(const QString &oldName, const QString &newName) {
    m_gitService->branchRename(oldName, newName).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// failure -> showErrorDialog()
void GitModule::branchDelete(const QString &name) {
    m_gitService->branchDelete(name).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

void GitModule::merge(const QString &ref) {
    m_gitService->merge(ref).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Merge Failed"), result.text());
    });
}

void GitModule::rebase(const QString &ref) {
    m_gitService->rebase(ref).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Rebase Failed"), result.text());
    });
}

// public: history

// success -> showGet()
// failure -> showErrorDialog()
void GitModule::logGet(const QString &ref) {
    m_gitService->logGet(ref).then(this, [this](const GitLogResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.text());
            return;
        }

        // assign each commit a lane and row
        QStringList lanes{};
        QHash<QString, QPoint> positions{};
        m_laneCount = 0;
        for (int row = 0; row < result.commits.size(); ++row) {
            const auto &commit = result.commits[row];
            int lane = static_cast<int>(lanes.indexOf(commit.hash));
            if (lane < 0) {
                lane = 0;
                lanes.prepend(commit.hash);
            }
            m_laneCount = qMax(m_laneCount, static_cast<int>(lanes.size()));
            positions.insert(commit.hash, QPoint(lane, row));
            lanes.removeAt(lane);
            for (const auto &parent: commit.parents) {
                if (!lanes.contains(parent)) lanes.insert(lane++, parent);
            }
        }

        // populate the table with the resolved parent positions
        m_log->removeRows(0, m_log->rowCount());
        for (const auto &commit: result.commits) {
            QVariantList parents{};
            for (const auto &hash: commit.parents) {
                const auto parent = positions.constFind(hash);
                if (parent != positions.cend()) parents.append(parent.value());
            }
            auto *node = new QStandardItem();
            node->setData(positions.value(commit.hash), PositionRole);
            node->setData(parents, ParentsRole);
            const QList items{node, new QStandardItem(commit.subject), new QStandardItem(commit.author), new QStandardItem(commit.relativeDate)};
            for (auto *item: items) item->setData(commit.hash, HashRole);
            m_log->appendRow(items);
        }
        emit logChanged();
        if (!result.commits.isEmpty()) showGet(result.commits.first().hash);
    });
}

// failure -> showErrorDialog()
void GitModule::showGet(const QString &hash) {
    m_gitService->showGet(hash).then(this, [this](const GitShowResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.text());
            return;
        }

        m_show->removeRows(0, m_show->rowCount());
        const QDir workTree(m_gitService->repoGet().workTreePath);
        QHash<QString, QStandardItem *> folders{};
        for (const auto &file: result.files) {
            const auto path = workTree.relativeFilePath(file.documentUrl.toLocalFile()).split('/');
            auto *parent = m_show->invisibleRootItem();
            QString folderPath{};
            for (qsizetype i = 0; i + 1 < path.size(); ++i) {
                folderPath += path.at(i) + '/';
                auto *&folder = folders[folderPath];
                if (!folder) {
                    folder = new QStandardItem(path.at(i));
                    folder->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                    parent->appendRow(folder);
                }
                parent = folder;
            }

            auto display = file.documentUrl.fileName();
            if (!file.originalUrl.isEmpty())
                display = QString("%1 -> %2 (%3%)").arg(file.originalUrl.fileName(), display, QString::number(file.similarity));
            auto *item = new QStandardItem(display);
            item->setData(uni_cast<QFileIcon>(file.documentUrl).value, Qt::DecorationRole);
            item->setData(file.status, StatusRole);
            parent->appendRow(item);
        }
        emit setCommitDetails('(' + result.shortHash + ')' + result.subject, result.date, result.author + '<' + result.email + '>');
    });
}

void GitModule::cherryPick(const QString &hash) {
    if (hash.isEmpty()) return;
    m_gitService->cherryPick(hash).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Cherry-pick Failed"), result.text());
    });
}

void GitModule::revert(const QString &hash) {
    if (hash.isEmpty()) return;
    m_gitService->revert(hash).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Revert Failed"), result.text());
    });
}

void GitModule::reset(const QString &hash, const GitResetMode::Value mode) {
    m_gitService->reset(hash, mode).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// public: working tree
void GitModule::statusGet() {
    m_gitService->statusGet().then(this, [this](const GitStatusResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.text());
            return;
        }

        m_workingTreeModel->clear();
        m_indexModel->clear();
        QHash<QString, QStandardItem *> workingTreeFolders{};
        QHash<QString, QStandardItem *> indexFolders{};
        const QDir workTree(m_gitService->repoGet().workTreePath);
        for (const auto &file: result.files) {
            const auto path = workTree.relativeFilePath(file.documentUrl.toLocalFile()).split('/');
            const auto appendFile = [&](const StatusModel *model, QHash<QString, QStandardItem *> &folders, GitFileStatus::Value status) {
                auto *parent = model->invisibleRootItem();
                QString folderPath{};
                for (qsizetype i = 0; i + 1 < path.size(); ++i) {
                    folderPath += path.at(i);
                    auto &folder = folders[folderPath];
                    if (!folder) {
                        folder = new QStandardItem(path.at(i));
                        folder->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                        folder->setData(QUrl::fromLocalFile(workTree.filePath(folderPath)), StatusModel::DocumentUrlRole);
                        parent->appendRow(folder);
                    }
                    parent = folder;
                    folderPath += '/';
                }
                auto *item = new QStandardItem(file.documentUrl.fileName());
                item->setData(uni_cast<QFileIcon>(file.documentUrl).value, Qt::DecorationRole);
                item->setData(file.documentUrl, StatusModel::DocumentUrlRole);
                item->setData(status, StatusModel::StatusRole);
                parent->appendRow(item);
            };
            if (file.workingTreeStatus != GitFileStatus::Unmodified)
                appendFile(m_workingTreeModel, workingTreeFolders, file.workingTreeStatus);
            if (file.indexStatus != GitFileStatus::Unmodified && file.indexStatus != GitFileStatus::Untracked)
                appendFile(m_indexModel, indexFolders, file.indexStatus);
        }
        emit expandChanges();
    });
}

QFuture<bool> GitModule::add(const QUrl &url) {
    return m_gitService->add(url).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
        else if (m_gitStatus != GitOperationState::Idle && m_gitStatus != GitOperationState::Transfer) operationRefresh();
        return result.success;
    });
}

void GitModule::untrack(const QUrl &url) {
    m_gitService->untrack(url).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

void GitModule::restore(const QUrl &url, const GitRestoreMode::Value mode) {
    m_gitService->restore(url, mode).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

void GitModule::ignore(const QUrl &url) {
    m_gitService->ignore(url).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

void GitModule::unignore(const QUrl &url) {
    m_gitService->unignore(url).then(this, [this](const GitResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
    });
}

// public: commit
void GitModule::gitCommitPre() {
    emit resetCommit();
    m_commitWindow->resize(1080, 720);
    m_commitWindow->show();
    statusGet();
}

void GitModule::gitCommit(const QString &subject) {
    m_commitWindow->close();
    processEnqueue(GitCommand::Commit, QStringList{"commit", "-m", subject});
}

// public: push
void GitModule::gitPushPre() {
    if (!m_upstream.isEmpty()) {
        gitAhead();
        return;
    }
    m_gitService->remotesRefresh().then(this, [this](const GitRemotesResult &result) {
        if (!result.success) emit showErrorDialog(tr("Git command failed"), result.text());
        else if (!m_upstream.isEmpty()) gitAhead();
        else if (result.remotes.isEmpty()) m_gitService->manage();
        else emit showErrorDialog(tr("No Upstream Branch"), tr("Right-click the remote branch to set it as the upstream."));
    });
}

void GitModule::gitAhead() {
    m_pushWindow->resize(1080, 720);
    m_pushWindow->show();
    processEnqueue(GitCommand::Ahead, QStringList{"log", "@{upstream}..HEAD", "-z", "--pretty=format:%h%x1e%s"});
}

void GitModule::gitDiff_() {
    processEnqueue(GitCommand::Diff_, QStringList{"diff", "--name-status", "@{upstream}..HEAD"});
}

void GitModule::gitShowCommit_(const QString &hash) {
    processEnqueue(GitCommand::ShowCommit_, QStringList{"show", hash, "--format=%h%x1e%s%x1e%ad%x1e%an%x1e%ae%x1e", "--name-status"});
}

// private
void GitModule::operationRefresh() {
    m_gitService->diffGet().then(this, [this](const GitStagedDiffResult &result) {
        QString operation{};
        switch (m_gitStatus) {
            case GitOperationState::Merge: operation = tr("Merging: ");
                break;
            case GitOperationState::Rebase: operation = tr("Rebasing: ");
                break;
            case GitOperationState::CherryPick: operation = tr("Cherry-picking: ");
                break;
            case GitOperationState::Revert: operation = tr("Reverting: ");
                break;
            default: return;
        }
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.text());
        } else if (!result.conflicts.isEmpty()) {
            emit openDocument(result.conflicts.first());
            emit refreshBackground(m_taskId, operation + tr("%1 file(s) left").arg(result.conflicts.size()));
        } else if ((m_gitStatus == GitOperationState::CherryPick || m_gitStatus == GitOperationState::Revert) && !result.hasChanges) {
            emit refreshBackground(m_taskId, operation + tr("no changes to commit"));
        } else {
            emit refreshBackground(m_taskId, operation + tr("ready to commit"));
            emit showContinueDialog();
        }
    });
}

void GitModule::processEnqueue(int command, const QStringList &arguments) {
    m_queue.enqueue(QVariantHash{
        {"command", command},
        {"arguments", arguments},
    });
    if (m_process->state() == QProcess::NotRunning) processDequeue();
}

void GitModule::processDequeue() {
    if (m_process->state() != QProcess::NotRunning) return;
    if (m_queue.isEmpty()) return;
    const auto &session = m_queue.dequeue();
    m_command = session["command"].toInt();
    m_process->start("git", session["arguments"].toStringList());
}

void GitModule::processFinished(const int exitcode) {
    const auto output = m_process->readAllStandardOutput();
    const auto error = m_process->readAllStandardError();
    const auto command = m_command;
    m_command = GitCommand::Null;
    // output
    if (exitcode == 0) {
        const QDir workTree(m_gitService->repoGet().workTreePath);
        // output parser
        switch (command) {
            case GitCommand::Ahead: {
                m_commitModel->clear();
                m_commitModel->appendRow(new QStandardItem(m_current + " -> " + m_upstream.section('/', 2)));
                auto *root = m_commitModel->item(0, 0);
                for (const auto &value: output.split('\0')) {
                    const auto param = value.split('\x1e');
                    if (param.size() < 2) continue;
                    const auto &hash = param[0];
                    const auto &subject = param[1];
                    auto *item = new QStandardItem(subject); // NOLINT
                    item->setData(hash, CommitModel::HashRole);
                    root->appendRow(item);
                }
                gitDiff_();
            }
            break;
            case GitCommand::Diff_: {
                m_showModel_->clear();
                QHash<QString, QStandardItem *> roots{};
                const auto &changes = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
                for (const auto &value: changes) {
                    const auto change = value.split('\t');
                    if (change.size() < 2) continue;
                    const auto status = GitFileStatus::fromCode(change[0].front());
                    QString path1{};
                    QString path2{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        path1 = change[1];
                        path2 = change[2];
                    } else {
                        path2 = change[1];
                    }
                    const auto &path = path2.split('/');
                    const auto &documentPath = workTree.filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(workTree.filePath(path1)).fileName(),
                            documentUrl.fileName(),
                            change[0].mid(1)
                        );
                    } else {
                        display = documentUrl.fileName();
                    }
                    const auto &source = uni_cast<QFileIcon>(documentUrl);

                    QStandardItem *rootItem{};
                    QString rootPath{};
                    for (int i = 0; i < path.size() - 1; ++i) {
                        if (!rootPath.isEmpty()) rootPath += '/';
                        rootPath += path[i];

                        auto *_rootItem = roots.value(rootPath);
                        if (!_rootItem) {
                            _rootItem = new QStandardItem(path[i]); // NOLINT
                            _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                            if (rootItem) rootItem->appendRow(_rootItem);
                            else m_showModel_->appendRow(_rootItem);
                            roots.insert(rootPath, _rootItem);
                        }
                        rootItem = _rootItem;
                    }

                    auto *item = new QStandardItem(display); // NOLINT
                    item->setData(source.value, Qt::DecorationRole);
                    item->setData(documentUrl, ShowModel::DocumentUrlRole);
                    item->setData(status, ShowModel::StatusRole);
                    if (rootItem) rootItem->appendRow(item);
                    else m_showModel_->appendRow(item);
                }
            }
            break;
            case GitCommand::ShowCommit_: {
                const auto param = output.split('\x1e');
                if (param.size() != 6) break;
                emit setPushCommitDetails(
                    '(' + QString::fromUtf8(param[0].trimmed()) + ')' + QString::fromUtf8(param[1].trimmed()),
                    QString::fromUtf8(param[2].trimmed()),
                    QString::fromUtf8(param[3].trimmed()) + '<' + QString::fromUtf8(param[4].trimmed()) + '>');

                m_showModel_->clear();
                QHash<QString, QStandardItem *> roots{};
                const auto &changes = QString::fromUtf8(param[5]).split('\n', Qt::SkipEmptyParts);
                for (const auto &value: changes) {
                    const auto change = value.split('\t');
                    if (change.size() < 2) continue;
                    const auto status = GitFileStatus::fromCode(change[0].front());
                    QString path1{};
                    QString path2{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        path1 = change[1];
                        path2 = change[2];
                    } else {
                        path2 = change[1];
                    }
                    const auto &path = path2.split('/');
                    const auto &documentPath = workTree.filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(workTree.filePath(path1)).fileName(),
                            documentUrl.fileName(),
                            change[0].mid(1)
                        );
                    } else {
                        display = documentUrl.fileName();
                    }
                    const auto &source = uni_cast<QFileIcon>(documentUrl);

                    QStandardItem *rootItem{};
                    QString rootPath{};
                    for (int i = 0; i < path.size() - 1; ++i) {
                        if (!rootPath.isEmpty()) rootPath += '/';
                        rootPath += path[i];

                        auto *_rootItem = roots.value(rootPath);
                        if (!_rootItem) {
                            _rootItem = new QStandardItem(path[i]); // NOLINT
                            _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                            if (rootItem) rootItem->appendRow(_rootItem);
                            else m_showModel_->appendRow(_rootItem);
                            roots.insert(rootPath, _rootItem);
                        }
                        rootItem = _rootItem;
                    }

                    auto *item = new QStandardItem(display); // NOLINT
                    item->setData(source.value, Qt::DecorationRole);
                    item->setData(documentUrl, ShowModel::DocumentUrlRole);
                    item->setData(status, ShowModel::StatusRole);
                    if (rootItem) rootItem->appendRow(item);
                    else m_showModel_->appendRow(item);
                }
            }
            break;
            default: break;
        }
    }
    // error
    else {
        const auto text = QString::fromUtf8(error).trimmed();
        if (!text.isEmpty()) emit showErrorDialog(tr("Git command failed"), text);
    }
    processDequeue();
}

// public
QHash<int, QByteArray> ShowModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[DocumentUrlRole] = "documentUrl";
    roles[StatusRole] = "status";
    return roles;
}

// public
StatusModel::StatusModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &StatusModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &StatusModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &StatusModel::emptyChanged);
}

QHash<int, QByteArray> StatusModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[DocumentUrlRole] = "documentUrl";
    roles[StatusRole] = "status";
    return roles;
}

// public
CommitModel::CommitModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &CommitModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &CommitModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &CommitModel::emptyChanged);
}

QHash<int, QByteArray> CommitModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[HashRole] = "hash";
    return roles;
}

#include "service/git/gitModule.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QQuickView>
#include <QQuickWidget>
#include <QTextStream>

#include "core/globalManager.h"
#include "mainWindow/mainWindow.h"
#include "service/git/gitConfig.h"
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
      m_repoWatcher(new QFileSystemWatcher(this)),
      m_repoWatcherTimer(new QTimer(this)),
      m_indexWatcher(new QFileSystemWatcher(this)),
      m_indexWatcherTimer(new QTimer(this)),
      m_branchWatcher(new QFileSystemWatcher(this)),
      m_branchWatcherTimer(new QTimer(this)),
      m_gitConfig(new GitConfig(this)),
      m_refs(new QStandardItemModel(this)),
      m_log(new QStandardItemModel(this)),
      m_showModel(new ShowModel(this)),
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

    connect(m_gitConfig, &GitConfig::showErrorDialog, this, &GitModule::showErrorDialog);
    connect(m_gitConfig, &GitConfig::showProxyDialog, this, &GitModule::showProxyDialog);

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_EDITOR", "true");
    m_process->setProcessEnvironment(env);
    m_process->setWorkingDirectory(g_workspaceUrl.toLocalFile());
    connect(m_process, &QProcess::finished, this, [this](const int exitcode) { processFinished(exitcode); });

    connect(m_repoWatcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_repoWatcherTimer->start(); });
    // connect(m_repoWatcher, &QFileSystemWatcher::directoryChanged, this, [](const QString &filePath) { qDebug() << "repo changed:" << filePath; });
    m_repoWatcherTimer->setSingleShot(true);
    m_repoWatcherTimer->setInterval(100);
    connect(m_repoWatcherTimer, &QTimer::timeout, this, &GitModule::gitRepo);

    connect(m_indexWatcher, &QFileSystemWatcher::fileChanged, this, [this] { m_indexWatcherTimer->start(); });
    // connect(m_indexWatcher, &QFileSystemWatcher::fileChanged, this, [](const QString &filePath) { qDebug() << "index changed:" << filePath; });
    m_indexWatcherTimer->setSingleShot(true);
    m_indexWatcherTimer->setInterval(100);
    connect(m_indexWatcherTimer, &QTimer::timeout, this, &GitModule::updateIndex);
    connect(m_indexWatcherTimer, &QTimer::timeout, this, &GitModule::gitStatus);

    connect(m_branchWatcher, &QFileSystemWatcher::fileChanged, this, [this] { m_branchWatcherTimer->start(); });
    // connect(m_branchWatcher, &QFileSystemWatcher::fileChanged, this, [](const QString &filePath) { qDebug() << "branch changed:" << filePath; });
    m_branchWatcherTimer->setSingleShot(true);
    m_branchWatcherTimer->setInterval(100);
    connect(m_branchWatcherTimer, &QTimer::timeout, this, &GitModule::refsGet);
}

GitModule::~GitModule() {
    delete m_commitWindow;
    delete m_pushWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void GitModule::propertySet() {
    m_widget->setInitialProperties({
        {"gitModule", QVariant::fromValue(this)},
        {"showModel", QVariant::fromValue(m_showModel)}
    });
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

    connect(m_gitService, &GitService::hasRepositoryChanged, this, &GitModule::gitWatch);
    gitWatch();
}

void GitModule::gitInit() {
    processEnqueue(GitCommand::Init, QStringList{"init"});
}

// public: commit
void GitModule::gitCommitPre() {
    emit resetCommit();
    m_commitWindow->resize(1080, 720);
    m_commitWindow->show();
    gitStatus();
}

void GitModule::gitStatus() {
    processEnqueue(GitCommand::Status, QStringList{"--no-optional-locks", "status", "-uall", "--porcelain"});
}

void GitModule::gitCommit(const QString &subject) {
    m_commitWindow->close();
    processEnqueue(GitCommand::Commit, QStringList{"commit", "-m", subject});
}

// public: pull
void GitModule::gitFetch() {
    processEnqueue(GitCommand::Fetch, QStringList{"fetch", "-p"});
    m_taskId = emit appendBackground(
        [this] { this->gitAbort(); },
        {});
    emit refreshBackground(m_taskId, tr("Fetching from remote..."));
    gitStatusSet(GitOperationState::Transfer);
}

// public: push
void GitModule::gitPushPre() {
    if (m_upstream.isEmpty()) gitRemoteGet();
    else gitAhead();
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

void GitModule::gitPush() {
    m_pushWindow->close();
    processEnqueue(GitCommand::Push, QStringList{"push"});
    m_taskId = emit appendBackground(
        [this] { this->gitAbort(); },
        {});
    emit refreshBackground(m_taskId, tr("Pushing to remote..."));
    gitStatusSet(GitOperationState::Transfer);
}

// public: file watcher
void GitModule::gitWatch() {
    if (m_gitService->rootPathGet().isEmpty()) return;
    m_process->setWorkingDirectory(m_gitService->rootPathGet());
    // repo watcher
    const QDir gitDir(m_gitService->gitDirPathGet());
    m_repoWatcher->addPath(gitDir.path());
    // index watcher
    const auto &indexPath = gitDir.filePath("index");
    if (QFileInfo::exists(indexPath)) m_indexWatcher->addPath(indexPath);
    // branch watcher
    const auto &files = m_branchWatcher->files();
    if (!files.isEmpty()) m_branchWatcher->removePaths(files);
    const auto &headPath = gitDir.filePath("HEAD");
    const auto &refsPath = gitDir.filePath("refs");
    if (QFileInfo::exists(headPath)) m_branchWatcher->addPath(headPath);
    if (QFileInfo::exists(refsPath)) {
        auto iterator = QDirIterator(refsPath, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext()) m_branchWatcher->addPath(iterator.next());
    }

    emit updateIndex();
    gitRepo();
    upstreamGet();
}

void GitModule::gitRepo() {
    if (m_gitStatus == GitOperationState::Transfer) return;
    const auto &gitDir = QDir(m_gitService->gitDirPathGet());
    // merge
    if (QFileInfo::exists(gitDir.filePath("MERGE_HEAD"))) {
        m_taskId = emit appendBackground(
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitOperationState::Merge);
    }
    // rebase
    else if (QFileInfo::exists(gitDir.filePath("REBASE_HEAD"))) {
        m_taskId = emit appendBackground(
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitOperationState::Rebase);
    }
    // cherry-pick
    else if (QFileInfo::exists(gitDir.filePath("CHERRY_PICK_HEAD"))) {
        m_taskId = emit appendBackground(
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitOperationState::CherryPick);
    }
    if (m_gitStatus != GitOperationState::Idle) gitDiff();
}

// public: branch
void GitModule::gitRemoteAdd(const QString &upstreamUrl) {
    processEnqueue(GitCommand::RemoteAdd, QStringList{"remote", "add", "origin", upstreamUrl});
}

void GitModule::gitRemoteGet() {
    processEnqueue(GitCommand::RemoteGet, QStringList{"remote"});
}

void GitModule::gitUpstreamSet(const QString &upstream) {
    processEnqueue(GitCommand::UpstreamSet, QStringList{"branch", "-u", upstream});
}

void GitModule::gitUpstreamUnset() {
    processEnqueue(GitCommand::UpstreamSet, QStringList{"branch", "--unset-upstream"});
}

// always -> refsGet()
void GitModule::upstreamGet() {
    m_gitService->upstreamGet().then(this, [this](const GitResult &result) {
        m_upstream = result.success ? result.output : QString{};
        refsGet();
    });
}

// success -> logGet()
// failure -> showErrorDialog()
void GitModule::refsGet() {
    m_gitService->refsGet().then(this, [this](const GitRefsResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.output);
            return;
        }

        m_refs->removeRows(0, m_refs->rowCount());
        m_current.clear();
        QHash<QString, QStandardItem *> groups{};
        for (const auto &[ref, current] : result.refs) {
            const auto kind = ref.section('/', 1, 1);
            const auto name = ref.section('/', 2);
            auto &group = groups[kind];
            if (!group) {
                group = new QStandardItem(kind == "heads" ? tr("Local") : kind == "remotes" ? tr("Remote") : tr("Tags"));
                group->setData(kind, TypeRole);
                m_refs->appendRow(group);
            }

            auto *item = new QStandardItem(name);
            item->setData(ref, RefRole);
            if (kind == "heads") {
                if (current) m_current = name;
                item->setData(current ? "current" : "local", TypeRole);
            } else if (kind == "remotes") {
                item->setData(ref == m_upstream ? "upstream" : "remote", TypeRole);
            } else {
                item->setData("tag", TypeRole);
            }
            if (ref == "refs/heads/master") group->insertRow(0, item);
            else group->appendRow(item);
        }
        emit expandBranches();
        if (!m_current.isEmpty()) logGet("refs/heads/" + m_current);
    });
}

void GitModule::gitSwitch(const QString &name) {
    processEnqueue(GitCommand::Switch, QStringList{"switch", name});
}

void GitModule::gitCreate(const QString &src, const QString &dst, const bool _switch) {
    QStringList arguments{};
    if (_switch) arguments = QStringList{"switch", "-c", dst, src};
    else arguments = QStringList{"branch", dst, src};
    processEnqueue(GitCommand::Create, arguments);
}

void GitModule::gitRename(const QString &src, const QString &dst) {
    processEnqueue(GitCommand::Rename, QStringList{"branch", "-m", src, dst});
}

void GitModule::gitDelete(const QString &name) {
    processEnqueue(GitCommand::Delete, QStringList{"branch", "-D", name});
}

// public: history
// success -> gitShowCommit()
// failure -> showErrorDialog()
void GitModule::logGet(const QString &ref) {
    m_gitService->logGet(ref).then(this, [this](const GitLogResult &result) {
        if (!result.success) {
            emit showErrorDialog(tr("Git command failed"), result.output);
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
            for (const auto &parent : commit.parents) {
                if (!lanes.contains(parent)) lanes.insert(lane++, parent);
            }
        }

        // populate the table with the resolved parent positions
        m_log->removeRows(0, m_log->rowCount());
        for (const auto &commit : result.commits) {
            QVariantList parents{};
            for (const auto &hash : commit.parents) {
                const auto parent = positions.constFind(hash);
                if (parent != positions.cend()) parents.append(parent.value());
            }
            auto *node = new QStandardItem();
            node->setData(positions.value(commit.hash), PositionRole);
            node->setData(parents, ParentsRole);
            const QList items{node, new QStandardItem(commit.subject), new QStandardItem(commit.author), new QStandardItem(commit.relativeDate)};
            for (auto *item : items) item->setData(commit.hash, HashRole);
            m_log->appendRow(items);
        }
        emit logChanged();
        if (!result.commits.isEmpty()) gitShowCommit(result.commits.first().hash);
    });
}

void GitModule::gitReset(const QString &hash, const GitResetMode::Value mode) {
    QString _mode{};
    switch (mode) {
        case GitResetMode::Mixed: _mode = "--mixed";
            break;
        case GitResetMode::Soft: _mode = "--soft";
            break;
        case GitResetMode::Hard: _mode = "--hard";
            break;
        case GitResetMode::Merge: _mode = "--merge";
            break;
        case GitResetMode::Keep: _mode = "--keep";
            break;
        default: return;
    }
    processEnqueue(GitCommand::Reset, QStringList{"reset", hash, _mode});
}

void GitModule::gitCherryPick(const QString &hash) {
    if (hash.isEmpty()) return;
    processEnqueue(GitCommand::CherryPick, QStringList{"cherry-pick", hash});
}

// public: show
void GitModule::gitShowCommit(const QString &hash) {
    processEnqueue(GitCommand::ShowCommit, QStringList{"show", hash, "--format=%h%x1e%s%x1e%ad%x1e%an%x1e%ae%x1e", "--name-status"});
}

void GitModule::gitShowFile(const QString &hash, const QUrl &documentUrl) {
    const auto &documentPath = documentUrl.toLocalFile();
    processEnqueue(GitCommand::ShowFile, QStringList{"show", hash, "--format=", documentPath});
}

void GitModule::gitMerge(const QString &name) {
    processEnqueue(GitCommand::Merge, QStringList{"merge", name});
}

void GitModule::gitRebase(const QString &name) {
    processEnqueue(GitCommand::Rebase, QStringList{"rebase", name});
}

void GitModule::gitAbort() {
    switch (m_gitStatus) {
        case GitOperationState::Transfer: m_process->terminate();
            break;
        case GitOperationState::Merge: processEnqueue(GitCommand::Abort, QStringList{"merge", "--abort"});
            break;
        case GitOperationState::Rebase: processEnqueue(GitCommand::Abort, QStringList{"rebase", "--abort"});
            break;
        case GitOperationState::CherryPick: processEnqueue(GitCommand::Abort, QStringList{"cherry-pick", "--abort"});
            break;
        default: break;
    }
}

void GitModule::gitContinue(const QString &message) {
    QStringList arguments{};
    switch (m_gitStatus) {
        case GitOperationState::Merge: {
            if (message.isEmpty()) arguments = {"merge", "--continue"};
            else arguments = {"commit", "-m", message};
        }
        break;
        case GitOperationState::Rebase: arguments = {"rebase", "--continue"};
            break;
        case GitOperationState::CherryPick: arguments = {"cherry-pick", "--continue"};
            break;
        default: break;
    }
    processEnqueue(GitCommand::Continue, arguments);
}

void GitModule::gitDiff() {
    processEnqueue(GitCommand::Diff, QStringList{"diff", "--name-only", "--diff-filter=U"});
}

// public: file
void GitModule::gitAdd(const QUrl &documentUrl) {
    QStringList arguments{};
    if (documentUrl.isEmpty()) {
        arguments = QStringList{"add", "--all"};
    } else {
        const auto &documentPath = documentUrl.toLocalFile();
        arguments = QStringList{"add", documentPath};
    }
    processEnqueue(GitCommand::Add, arguments);
}

void GitModule::gitRestore(const QUrl &documentUrl, const GitRestoreMode::Value mode) {
    QStringList arguments{"restore"};
    switch (mode) {
        case GitRestoreMode::Worktree: arguments << "--worktree";
            break;
        case GitRestoreMode::Staged: arguments << "--staged";
            break;
        case GitRestoreMode::Both: arguments << "--worktree" << "--staged";
            break;
        default: return;
    }
    if (documentUrl.isEmpty()) {
        arguments << ":/";
    } else {
        const auto documentPath = documentUrl.toLocalFile();
        arguments << documentPath;
    }
    processEnqueue(GitCommand::Restore, arguments);
}

void GitModule::gitIgnore(const QUrl &documentUrl, const bool status) {
    // check file and open
    const auto gitignorePath = QDir(m_gitService->rootPathGet()).filePath(".gitignore");
    auto gitignoreFile = QFile(gitignorePath);
    if (!gitignoreFile.exists()) {
        if (!gitignoreFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        gitignoreFile.close();
    }
    // read to string list
    QStringList gitignoreList{};
    if (!gitignoreFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&gitignoreFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        gitignoreList.append(line);
    }
    gitignoreFile.close();
    // ready to add / remove
    const auto documentPath = documentUrl.toLocalFile();
    const auto workspaceDir = QDir(m_gitService->rootPathGet());
    const auto relativePath = '/' + workspaceDir.relativeFilePath(documentPath);
    if (status) {
        bool inserted = false;
        for (int i = 0; i < gitignoreList.size(); ++i) {
            if (gitignoreList[i].startsWith('#') || gitignoreList[i].trimmed().isEmpty()) continue;
            if (relativePath == gitignoreList[i]) break;
            if (relativePath < gitignoreList[i]) {
                gitignoreList.insert(i, relativePath);
                inserted = true;
                break;
            }
        }
        if (!inserted) gitignoreList.append(relativePath);
    } else {
        gitignoreList.removeAll(relativePath);
    }
    // write back
    if (!gitignoreFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&gitignoreFile);
    for (const QString &line: gitignoreList) {
        out << line << "\n";
    }
    gitignoreFile.close();

    if (status) {
        QStringList args{"rm"};
        if (QFileInfo(documentPath).isDir())args << "-r";
        args << "--cached" << "--ignore-unmatch" << "--" << documentPath;
        processEnqueue(GitCommand::Ignore, args);
    } else {
        emit updateIndex();
    }
}

// git config
void GitModule::gitProxyGet() const {
    m_gitConfig->localHttpProxyGet();
}

void GitModule::gitProxySet(const QString &localHttpProxy, const QString &localHttpsProxy, const QString &globalHttpProxy, const QString &globalHttpsProxy) const {
    m_gitConfig->gitProxySet(localHttpProxy, localHttpsProxy, globalHttpProxy, globalHttpsProxy);
}

// private
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
        const QDir rootDir(m_gitService->rootPathGet());
        // output parser
        switch (command) {
            case GitCommand::ShowCommit: {
                const auto param = output.split('\x1e');
                if (param.size() != 6) break;
                const auto &hash = QString::fromUtf8(param[0].trimmed());
                emit setCommitDetails(
                    '(' + hash + ')' + QString::fromUtf8(param[1].trimmed()),
                    QString::fromUtf8(param[2].trimmed()),
                    QString::fromUtf8(param[3].trimmed()) + '<' + QString::fromUtf8(param[4].trimmed()) + '>');

                m_showModel->clear();
                m_showModel->hashSet(QString());
                m_showModel->hashSet(hash);
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
                    const auto &documentPath = rootDir.filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(rootDir.filePath(path1)).fileName(),
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
                            else m_showModel->appendRow(_rootItem);
                            roots.insert(rootPath, _rootItem);
                        }
                        rootItem = _rootItem;
                    }

                    auto *item = new QStandardItem(display); // NOLINT
                    item->setData(source.value, Qt::DecorationRole);
                    item->setData(documentUrl, ShowModel::DocumentUrlRole);
                    item->setData(status, ShowModel::StatusRole);
                    if (rootItem) rootItem->appendRow(item);
                    else m_showModel->appendRow(item);
                }
            }
            break;
            case GitCommand::ShowFile: {
                qDebug() << output;
            }
            break;
            case GitCommand::Diff: {
                const auto &paths = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
                QString operation{};
                switch (m_gitStatus) {
                    case GitOperationState::Merge: operation = tr("Merging: ");
                        break;
                    case GitOperationState::Rebase: operation = tr("Rebasing: ");
                        break;
                    case GitOperationState::CherryPick: operation = tr("Cherry-picking: ");
                        break;
                    default:
                        return;
                }
                // finish conflict resolve
                if (paths.size() == 0) {
                    emit refreshBackground(m_taskId, operation + tr("ready to commit"));
                    emit showContinueDialog();
                }
                // continue conflict resolve
                else {
                    const auto &documentPath = rootDir.filePath(paths.first());
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    emit openDocument(documentUrl);
                    emit refreshBackground(m_taskId, operation + tr("%1 file(s) left").arg(QString::number(paths.size())));
                }
            }
            break;
            case GitCommand::Status: {
                m_workingTreeModel->clear();
                m_indexModel->clear();
                QHash<QString, QStandardItem *> workingTreeRoots{};
                QHash<QString, QStandardItem *> indexRoots{};

                const auto changes = output.split('\n');
                for (const auto &value: changes) {
                    if (value.size() < 4) continue;
                    const auto &change = QString::fromUtf8(value);

                    const auto indexStatus = GitFileStatus::fromCode(change.at(0));
                    const auto workingTreeStatus = GitFileStatus::fromCode(change.at(1));
                    auto path = change.mid(3).trimmed();
                    if (indexStatus == GitFileStatus::Renamed || indexStatus == GitFileStatus::Copied || workingTreeStatus == GitFileStatus::Renamed || workingTreeStatus ==
                        GitFileStatus::Copied)
                        path = path.section(" -> ", 1);
                    const auto &documentPath = rootDir.filePath(path);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    const auto &display = documentUrl.fileName();
                    const auto &source = uni_cast<QFileIcon>(documentUrl);
                    const auto &pathList = path.split('/');

                    // append working tree model
                    if (workingTreeStatus != GitFileStatus::Unmodified) {
                        QStandardItem *rootItem{};
                        QString rootPath{};
                        for (int i = 0; i < pathList.size() - 1; ++i) {
                            if (!rootPath.isEmpty()) rootPath += '/';
                            rootPath += pathList[i];

                            auto *_rootItem = workingTreeRoots.value(rootPath);
                            if (!_rootItem) {
                                _rootItem = new QStandardItem(pathList[i]); // NOLINT
                                _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                                _rootItem->setData(QUrl::fromLocalFile(rootDir.filePath(rootPath)), StatusModel::DocumentUrlRole);

                                if (rootItem) rootItem->appendRow(_rootItem);
                                else m_workingTreeModel->appendRow(_rootItem);
                                workingTreeRoots.insert(rootPath, _rootItem);
                            }
                            rootItem = _rootItem;
                        }

                        auto *item = new QStandardItem(display); // NOLINT
                        item->setData(source.value, Qt::DecorationRole);
                        item->setData(documentUrl, StatusModel::DocumentUrlRole);
                        item->setData(workingTreeStatus, StatusModel::StatusRole);
                        if (rootItem) rootItem->appendRow(item);
                        else m_workingTreeModel->appendRow(item);

                        // skip index model if not added
                        if (workingTreeStatus == GitFileStatus::Untracked) continue;
                    }

                    // append index model
                    if (indexStatus != GitFileStatus::Unmodified) {
                        QStandardItem *rootItem{};
                        QString rootPath{};
                        for (int i = 0; i < pathList.size() - 1; ++i) {
                            if (!rootPath.isEmpty()) rootPath += '/';
                            rootPath += pathList[i];

                            auto *_rootItem = indexRoots.value(rootPath);
                            if (!_rootItem) {
                                _rootItem = new QStandardItem(pathList[i]); // NOLINT
                                _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                                _rootItem->setData(QUrl::fromLocalFile(rootDir.filePath(rootPath)), StatusModel::DocumentUrlRole);

                                if (rootItem) rootItem->appendRow(_rootItem);
                                else m_indexModel->appendRow(_rootItem);
                                indexRoots.insert(rootPath, _rootItem);
                            }
                            rootItem = _rootItem;
                        }

                        auto *item = new QStandardItem(display); // NOLINT
                        item->setData(source.value, Qt::DecorationRole);
                        item->setData(documentUrl, StatusModel::DocumentUrlRole);
                        item->setData(indexStatus, StatusModel::StatusRole);
                        if (rootItem) rootItem->appendRow(item);
                        else m_indexModel->appendRow(item);
                    }
                }
                emit expandChanges();
            }
            break;
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
                    const auto &documentPath = rootDir.filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(rootDir.filePath(path1)).fileName(),
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
                    const auto &documentPath = rootDir.filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitFileStatus::Renamed || status == GitFileStatus::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(rootDir.filePath(path1)).fileName(),
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
        // state machine
        switch (command) {
            case GitCommand::Init: {
                m_gitService->repositoryGet();
            }
            break;
            case GitCommand::Add: {
                if (m_current.isEmpty()) {
                    gitWatch();
                    gitStatus();
                }
                if (m_gitStatus != GitOperationState::Idle) {
                    emit addFinish();
                    gitDiff();
                }
            }
            break;
            case GitCommand::Commit: {
                if (m_current.isEmpty()) {
                    gitWatch();
                    gitStatus();
                }
            }
            break;
            case GitCommand::Fetch:
            case GitCommand::Continue:
            case GitCommand::Abort:
            case GitCommand::Push: {
                emit removeBackground(m_taskId);
                gitStatusSet(GitOperationState::Idle);
                if (command == GitCommand::Fetch) gitWatch();
            }
            break;
            case GitCommand::Ahead: gitDiff_();
                break;
            case GitCommand::RemoteAdd: gitFetch();
                break;
            case GitCommand::RemoteGet: {
                if (!m_upstream.isEmpty()) gitAhead();
                else if (output.isEmpty()) emit showRemoteAddDialog();
                else emit showErrorDialog(tr("No Upstream Branch"), tr("Right-click the remote branch to set it as the upstream."));
            }
            break;
            case GitCommand::UpstreamUnset:
            case GitCommand::UpstreamSet: upstreamGet();
                break;
            case GitCommand::Switch: upstreamGet();
                break;
            case GitCommand::Create:
            case GitCommand::Rename:
            case GitCommand::Delete:
            case GitCommand::CherryPick: gitWatch();
                break;
            default: break;
        }
    }
    // error
    else {
        QString title{};
        QString text{};
        // error parser
        switch (command) {
            case GitCommand::Merge: {
                title = tr("Merge Failed");
                text = QString::fromUtf8(output).trimmed();
            }
            break;
            case GitCommand::Rebase: {
                title = tr("Rebase Failed");
                if (!output.isEmpty()) text = QString::fromUtf8(output).trimmed();
                else text = QString::fromUtf8(error).trimmed();
            }
            break;
            case GitCommand::CherryPick: {
                title = tr("Cherry-pick Failed");
                if (!output.isEmpty()) text = QString::fromUtf8(output).trimmed();
                else text = QString::fromUtf8(error).trimmed();
            }
            break;
            case GitCommand::Fetch: {
                title = tr("Fetch Failed");
                text = QString::fromUtf8(error).trimmed();
                emit removeBackground(m_taskId);
                gitStatusSet(GitOperationState::Idle);
            }
            break;
            case GitCommand::Push: {
                title = tr("Push Failed");
                text = QString::fromUtf8(error).trimmed();
                emit removeBackground(m_taskId);
                gitStatusSet(GitOperationState::Idle);
            }
            break;
            default: {
                title = tr("Git command failed");
                text = QString::fromUtf8(error).trimmed();
            }
            break;
        }
        if (!title.isEmpty() && !text.isEmpty()) emit showErrorDialog(title, text);
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

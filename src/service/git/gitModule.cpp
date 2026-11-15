#include "service/git/gitModule.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QTimer>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWidget>
#include <QTextDocument>

#include "core/globalManager.h"
#include "mainWindow/mainWindow.h"
#include "service/git/gitConfig.h"
#include "util/uniCast.h"

// public
GitModule::GitModule()
    : DockWidget("Git"),
      m_gitEnabled(gitGet()),
      m_gitStatus(GitStatus::Idle),
      m_config(g_workspaceConfig["gitConfig"].toObject()),
      m_widget(new QQuickWidget()),
      m_commitWindow(new QQuickView()),
      m_pushWindow(new QQuickView()),
      m_process(new QProcess(this)),
      m_repoWatcher(new QFileSystemWatcher(this)),
      m_repoWatcherTimer(new QTimer(this)),
      m_indexWatcher(new QFileSystemWatcher(this)),
      m_indexWatcherTimer(new QTimer(this)),
      m_branchWatcher(new QFileSystemWatcher(this)),
      m_branchWatcherTimer(new QTimer(this)),
      m_gitConfig(new GitConfig(this)),
      m_branchModel(new BranchModel(this)),
      m_logModel(new LogModel(this)),
      m_showModel(new ShowModel(this)),
      m_workingTreeModel(new StatusModel(this)),
      m_indexModel(new StatusModel(this)),
      m_commitModel(new CommitModel(this)),
      m_showModel_(new ShowModel(this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);

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
    connect(m_branchWatcherTimer, &QTimer::timeout, this, &GitModule::gitBranch);
}

GitModule::~GitModule() {
    delete m_commitWindow;
    delete m_pushWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void GitModule::propertySet(const QVariantHash &objects) {
    m_continueDialog = qvariant_cast<QObject *>(objects["gitModuleContinueDialog"]);
    m_errorDialog = qvariant_cast<QObject *>(objects["gitModuleErrorDialog"]);
    m_remoteAddDialog = qvariant_cast<QObject *>(objects["gitModuleRemoteAddDialog"]);

    m_widget->rootContext()->setContextProperty("gitModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_widget->rootContext()->setContextProperty("branchMenu", objects["gitModuleBranchMenu"]);
    m_widget->rootContext()->setContextProperty("logMenu", objects["gitModuleLogMenu"]);
    m_widget->rootContext()->setContextProperty("branchModel", m_branchModel);
    m_widget->rootContext()->setContextProperty("logModel", m_logModel);
    m_widget->rootContext()->setContextProperty("showModel", m_showModel);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/service/git/gitModule.qml"));
    m_root = m_widget->rootObject();
    if (m_gitEnabled) gitWatch();

    // config
    m_gitConfig->propertySet(QVariantHash{
        {"gitModuleErrorDialog", objects["gitModuleErrorDialog"]},
        {"gitModuleProxyDialog", objects["gitModuleProxyDialog"]},
    });

    // commit window
    m_commitWindow->setTitle(tr("Git Commit"));
    m_commitWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());

    m_commitWindow->rootContext()->setContextProperty("gitModule", this);
    m_commitWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_commitWindow->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_commitWindow->rootContext()->setContextProperty("workingTreeModel", m_workingTreeModel);
    m_commitWindow->rootContext()->setContextProperty("indexModel", m_indexModel);

    m_commitWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_commitWindow->setSource(QUrl("qrc:/qml/service/git/gitCommitWindow.qml"));
    m_commitRoot = m_commitWindow->rootObject();

    // push window
    m_pushWindow->setTitle(tr("Git Push"));
    m_pushWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());

    m_pushWindow->rootContext()->setContextProperty("gitModule", this);
    m_pushWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_pushWindow->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_pushWindow->rootContext()->setContextProperty("commitModel", m_commitModel);
    m_pushWindow->rootContext()->setContextProperty("showModel", m_showModel_);

    m_pushWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_pushWindow->setSource(QUrl("qrc:/qml/service/git/gitPushWindow.qml"));
}

void GitModule::propertyGet(const QVariantMap &objects) {
    m_canvas = qvariant_cast<QObject *>(objects["canvas"]);
    m_subjectLabel = qvariant_cast<QObject *>(objects["subjectLabel"]);
    m_dateLabel = qvariant_cast<QObject *>(objects["dateLabel"]);
    m_authorLabel = qvariant_cast<QObject *>(objects["authorLabel"]);
}

void GitModule::propertyGet_(const QVariantMap &objects) {
    m_subjectLabel_ = qvariant_cast<QObject *>(objects["subjectLabel"]);
    m_dateLabel_ = qvariant_cast<QObject *>(objects["dateLabel"]);
    m_authorLabel_ = qvariant_cast<QObject *>(objects["authorLabel"]);
}

void GitModule::branchSet(const QString &name) {
    if (m_current == name) return;
    m_current = name;
    gitLog();
}

bool GitModule::gitGet() {
    QProcess process{};
    process.start("git", {"--version"});
    process.waitForFinished();
    if (process.exitCode() == 0) {
        process.setWorkingDirectory(g_workspaceUrl.toLocalFile());
        process.start("git", {"status"});
        process.waitForFinished();
        if (process.exitCode() == 0) {
            return true;
        }
    }
    return false;
}

void GitModule::gitInit() {
    processEnqueue(GitCommand::Init, QStringList{"init"});
}

// public: commit
void GitModule::gitCommitPre() {
    QMetaObject::invokeMethod(m_commitRoot, "reset");
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
    emit appendBackground(
        m_taskId,
        [this] { this->gitAbort(); },
        {});
    emit refreshBackground(m_taskId, tr("Fetching from remote..."));
    gitStatusSet(GitStatus::Transfer);
}

// public: push
void GitModule::gitPushPre() {
    if (!m_remote) {
        QMetaObject::invokeMethod(m_remoteAddDialog, "open");
    } else if (m_upstream.isEmpty()) {
        m_errorDialog->setProperty("title", tr("No Upstream Branch"));
        m_errorDialog->setProperty("text", tr("Right-click the remote branch to set it as the upstream."));
        QMetaObject::invokeMethod(m_errorDialog, "open");
    } else {
        gitAhead();
    }
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
    emit appendBackground(
        m_taskId,
        [this] { this->gitAbort(); },
        {});
    emit refreshBackground(m_taskId, tr("Pushing to remote..."));
    gitStatusSet(GitStatus::Transfer);
}

// public: file watcher
void GitModule::gitWatch() {
    processEnqueue(GitCommand::Watch, QStringList{"rev-parse", "--show-toplevel", "--absolute-git-dir"});
}

void GitModule::gitRepo() {
    if (m_gitStatus == GitStatus::Transfer) return;
    const auto &gitDir = QDir(m_gitDirPath);
    // merge
    if (QFileInfo::exists(gitDir.filePath("MERGE_HEAD"))) {
        emit appendBackground(
            m_taskId,
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitStatus::Merge);
    }
    // rebase
    else if (QFileInfo::exists(gitDir.filePath("REBASE_HEAD"))) {
        emit appendBackground(
            m_taskId,
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitStatus::Rebase);
    }
    // cherry-pick
    else if (QFileInfo::exists(gitDir.filePath("CHERRY_PICK_HEAD"))) {
        emit appendBackground(
            m_taskId,
            [this] { this->gitAbort(); },
            [this] { this->gitDiff(); });
        gitStatusSet(GitStatus::CherryPick);
    }
    if (m_gitStatus != GitStatus::Idle) gitDiff();
}

// public: branch
void GitModule::gitRemoteAdd(const QString &upstreamUrl) {
    processEnqueue(GitCommand::RemoteAdd, QStringList{"remote", "add", "origin", upstreamUrl});
}

void GitModule::gitRemoteGet() {
    processEnqueue(GitCommand::RemoteGet, QStringList{"remote", "-v"});
}

void GitModule::gitUpstreamSet(const QString &upstream) {
    processEnqueue(GitCommand::UpstreamSet, QStringList{"branch", "-u", upstream});
}

void GitModule::gitUpstreamUnset() {
    processEnqueue(GitCommand::UpstreamSet, QStringList{"branch", "--unset-upstream"});
}

void GitModule::gitUpstreamGet() {
    processEnqueue(GitCommand::UpstreamGet, QStringList{"rev-parse", "--abbrev-ref", "@{upstream}"});
}

void GitModule::gitBranch() {
    processEnqueue(GitCommand::Branch, QStringList{"for-each-ref", "--format=%(HEAD)%1e%(refname)%1e%(objectname:short)%1e%(subject)%1e%(symref)"});
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

// public: log
void GitModule::gitLog() {
    if (m_current.isEmpty()) return;
    processEnqueue(GitCommand::Log, QStringList{"log", m_current, "-z", "--pretty=format:%h%x1e%p%x1e%ar%x1e%an%x1e%s", "--"});
}

void GitModule::gitReset(const QString &hash, const int mode) {
    QString _mode{};
    switch (mode) {
        case ResetMode::Mixed: _mode = "--mixed";
            break;
        case ResetMode::Soft: _mode = "--soft";
            break;
        case ResetMode::Hard: _mode = "--hard";
            break;
        case ResetMode::Merge: _mode = "--merge";
            break;
        case ResetMode::Keep: _mode = "--keep";
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
        case GitStatus::Transfer: m_process->terminate();
            break;
        case GitStatus::Merge: processEnqueue(GitCommand::Abort, QStringList{"merge", "--abort"});
            break;
        case GitStatus::Rebase: processEnqueue(GitCommand::Abort, QStringList{"rebase", "--abort"});
            break;
        case GitStatus::CherryPick: processEnqueue(GitCommand::Abort, QStringList{"cherry-pick", "--abort"});
            break;
        default: break;
    }
}

void GitModule::gitContinue(const QString &message) {
    QStringList arguments{};
    switch (m_gitStatus) {
        case GitStatus::Merge: {
            if (message.isEmpty()) arguments = {"merge", "--continue"};
            else arguments = {"commit", "-m", message};
        }
        break;
        case GitStatus::Rebase: arguments = {"rebase", "--continue"};
            break;
        case GitStatus::CherryPick: arguments = {"cherry-pick", "--continue"};
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

void GitModule::gitRestore(const QUrl &documentUrl, const int mode) {
    QStringList arguments{"restore"};
    switch (mode) {
        case RestoreMode::Worktree: arguments << "--worktree";
            break;
        case RestoreMode::Staged: arguments << "--staged";
            break;
        case RestoreMode::Both: arguments << "--worktree" << "--staged";
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
    const auto gitignorePath = QDir(g_gitRootPath).filePath(".gitignore");
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
    const auto workspaceDir = QDir(g_gitRootPath);
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
        // output parser
        switch (command) {
            case GitCommand::Watch: {
                const auto &lines = output.trimmed().split('\n');
                if (lines.size() != 2) return;
                // git working tree
                g_gitRootPath = QString::fromUtf8(lines[0]).trimmed();
                m_process->setWorkingDirectory(g_gitRootPath);
                // repo watcher
                m_gitDirPath = QString::fromUtf8(lines[1]).trimmed();
                if (QFileInfo::exists(m_gitDirPath)) m_repoWatcher->addPath(m_gitDirPath);
                const auto &gitDir = QDir(m_gitDirPath);
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
            }
            break;
            case GitCommand::RemoteGet: m_remote = !QString::fromUtf8(output).trimmed().isEmpty();
                break;
            case GitCommand::UpstreamGet: m_upstream = QString::fromUtf8(output).trimmed();
                break;
            case GitCommand::Branch: {
                m_branchModel->clear();
                m_current.clear();
                QStandardItem *localItem = nullptr;
                QStandardItem *remoteItem = nullptr;
                for (const auto &value: output.split('\n')) {
                    const auto param = value.split('\x1e');
                    if (param.size() != 5 || !param[4].isEmpty()) continue; // exclude symbolic refs such as origin/HEAD

                    const auto ref = QString::fromUtf8(param[1]);
                    QString name{};
                    QString type{};
                    if (ref.startsWith("refs/heads/")) {
                        name = ref.mid(11);
                        type = param[0] == "*" ? m_current = name, "current" : "local";
                    } else if (ref.startsWith("refs/remotes/")) {
                        name = ref.mid(13);
                        type = name == m_upstream ? "upstream" : "remote";
                    } else {
                        qDebug() << QString("contact author: unsupported git ref (%1)").arg(name);
                        continue;
                    }

                    const auto hash = QString::fromUtf8(param[2]);
                    const auto commit = QString::fromUtf8(param[3]);
                    auto *item = new QStandardItem(name); // NOLINT
                    item->setData(type, BranchModel::TypeRole);
                    item->setData(hash, BranchModel::HashRole);
                    item->setData(commit, BranchModel::CommitRole);
                    // local
                    if (type == "current" || type == "local") {
                        if (localItem == nullptr) {
                            localItem = new QStandardItem(tr("Local")); // NOLINT
                            localItem->setData("localRep", BranchModel::TypeRole);
                            m_branchModel->appendRow(localItem);
                        }
                        if (name == "master") localItem->insertRow(0, item);
                        else localItem->appendRow(item);
                    } else {
                        if (remoteItem == nullptr) {
                            remoteItem = new QStandardItem(tr("Remote")); // NOLINT
                            remoteItem->setData("remoteRep", BranchModel::TypeRole);
                            m_branchModel->appendRow(remoteItem);
                        }
                        remoteItem->appendRow(item);
                    }
                }
                QMetaObject::invokeMethod(m_root, "branchExpand");
            }
            break;
            case GitCommand::Log: {
                m_logModel->clear();
                QHash<QString, QPoint> nodeHash{};
                QList<QStringList> parentRows{};
                QStringList lanes{};
                int laneCount = 0;
                for (const auto &value: output.split('\0')) {
                    const auto param = value.split('\x1e');
                    if (param.size() != 5) continue;

                    const QString &hash = QString::fromUtf8(param[0]);
                    const QStringList parents = QString::fromUtf8(param[1]).split(' ', Qt::SkipEmptyParts);
                    int lane = static_cast<int>(lanes.indexOf(hash));
                    if (lane < 0) {
                        lane = 0;
                        lanes.insert(0, hash);
                    }
                    const int index = m_logModel->rowCount();
                    const QPoint nodePos(lane, index);
                    nodeHash.insert(hash, nodePos);
                    parentRows.append(parents);

                    lanes.removeAt(lane);
                    int insertLane = lane;
                    for (const QString &parentHash: parents) {
                        if (lanes.contains(parentHash)) continue;
                        lanes.insert(insertLane++, parentHash);
                    }
                    laneCount = qMax(laneCount, static_cast<int>(lanes.size()));

                    const auto &date = QString::fromUtf8(param[2]);
                    const auto &author = QString::fromUtf8(param[3]);
                    const auto subject = QString::fromUtf8(param[4]);
                    const auto &nodeItem = new QStandardItem(); // NOLINT
                    const QList items{new QStandardItem(date), new QStandardItem(author), new QStandardItem(subject), nodeItem};
                    for (auto *item: items) {
                        item->setData(hash, LogModel::HashRole);
                    }
                    nodeItem->setData(nodePos, LogModel::PositionRole);
                    m_logModel->appendRow(items);
                }

                for (int row = 0; row < parentRows.size(); ++row) {
                    QVariantList parentPositions{};
                    for (const QString &parentHash: parentRows[row]) {
                        const auto parent = nodeHash.constFind(parentHash);
                        if (parent != nodeHash.cend()) parentPositions.append(parent.value());
                    }
                    m_logModel->item(row, 3)->setData(parentPositions, LogModel::ParentRole);
                }
                m_canvas->setProperty("laneCount", qMax(laneCount, 1));
            }
            break;
            case GitCommand::ShowCommit: {
                const auto param = output.split('\x1e');
                if (param.size() != 6) break;
                const auto &hash = QString::fromUtf8(param[0].trimmed());
                m_subjectLabel->setProperty("text", '(' + hash + ')' + QString::fromUtf8(param[1].trimmed()));
                m_dateLabel->setProperty("text", QString::fromUtf8(param[2].trimmed()));
                m_authorLabel->setProperty("text", QString::fromUtf8(param[3].trimmed()) + '<' + QString::fromUtf8(param[4].trimmed()) + '>');

                m_showModel->clear();
                m_showModel->hashSet(QString());
                m_showModel->hashSet(hash);
                QHash<QString, QStandardItem *> roots{};
                const auto &changes = QString::fromUtf8(param[5]).split('\n', Qt::SkipEmptyParts);
                for (const auto &value: changes) {
                    const auto change = value.split('\t');
                    if (change.size() < 2) continue;
                    const auto &status = g_gitStatusCode[change[0].front()];
                    QString path1{};
                    QString path2{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        path1 = change[1];
                        path2 = change[2];
                    } else {
                        path2 = change[1];
                    }
                    const auto &path = path2.split('/');
                    const auto &documentPath = QDir(g_gitRootPath).filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(QDir(g_gitRootPath).filePath(path1)).fileName(),
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
                    case GitStatus::Merge: operation = tr("Merging: ");
                        break;
                    case GitStatus::Rebase: operation = tr("Rebasing: ");
                        break;
                    case GitStatus::CherryPick: operation = tr("Cherry-picking: ");
                        break;
                    default:
                        return;
                }
                // finish conflict resolve
                if (paths.size() == 0) {
                    emit refreshBackground(m_taskId, operation + tr("ready to commit"));
                    QMetaObject::invokeMethod(m_continueDialog, "open");
                }
                // continue conflict resolve
                else {
                    const auto &documentPath = QDir(g_gitRootPath).filePath(paths.first());
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

                    const auto indexStatus = g_gitStatusCode[change.at(0)];
                    const auto workingTreeStatus = g_gitStatusCode[change.at(1)];
                    auto path = change.mid(3).trimmed();
                    if (indexStatus == 'R' || indexStatus == 'C' || workingTreeStatus == 'R' || workingTreeStatus == 'C') path = path.section(" -> ", 1);
                    const auto &documentPath = QDir(g_gitRootPath).filePath(path);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    const auto &display = documentUrl.fileName();
                    const auto &source = uni_cast<QFileIcon>(documentUrl);
                    const auto &pathList = path.split('/');

                    // append working tree model
                    if (workingTreeStatus != GitStatusCode::Unmodified) {
                        QStandardItem *rootItem{};
                        QString rootPath{};
                        for (int i = 0; i < pathList.size() - 1; ++i) {
                            if (!rootPath.isEmpty()) rootPath += '/';
                            rootPath += pathList[i];

                            auto *_rootItem = workingTreeRoots.value(rootPath);
                            if (!_rootItem) {
                                _rootItem = new QStandardItem(pathList[i]); // NOLINT
                                _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                                _rootItem->setData(QUrl::fromLocalFile(QDir(g_gitRootPath).filePath(rootPath)), StatusModel::DocumentUrlRole);

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
                        if (workingTreeStatus == GitStatusCode::Untracked) continue;
                    }

                    // append index model
                    if (indexStatus != GitStatusCode::Unmodified) {
                        QStandardItem *rootItem{};
                        QString rootPath{};
                        for (int i = 0; i < pathList.size() - 1; ++i) {
                            if (!rootPath.isEmpty()) rootPath += '/';
                            rootPath += pathList[i];

                            auto *_rootItem = indexRoots.value(rootPath);
                            if (!_rootItem) {
                                _rootItem = new QStandardItem(pathList[i]); // NOLINT
                                _rootItem->setData(QUrl("qrc:/icon/fileTypeFolder.svg"), Qt::DecorationRole);
                                _rootItem->setData(QUrl::fromLocalFile(QDir(g_gitRootPath).filePath(rootPath)), StatusModel::DocumentUrlRole);

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
                QMetaObject::invokeMethod(m_commitRoot, "workingTreeExpand");
                QMetaObject::invokeMethod(m_commitRoot, "indexExpand");
            }
            break;
            case GitCommand::Ahead: {
                m_commitModel->clear();
                m_commitModel->appendRow(new QStandardItem(m_current + " -> " + m_upstream));
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
                    const auto &status = g_gitStatusCode[change[0].front()];
                    QString path1{};
                    QString path2{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        path1 = change[1];
                        path2 = change[2];
                    } else {
                        path2 = change[1];
                    }
                    const auto &path = path2.split('/');
                    const auto &documentPath = QDir(g_gitRootPath).filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(QDir(g_gitRootPath).filePath(path1)).fileName(),
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
                m_subjectLabel_->setProperty("text", '(' + QString::fromUtf8(param[0].trimmed()) + ')' + QString::fromUtf8(param[1].trimmed()));
                m_dateLabel_->setProperty("text", QString::fromUtf8(param[2].trimmed()));
                m_authorLabel_->setProperty("text", QString::fromUtf8(param[3].trimmed()) + '<' + QString::fromUtf8(param[4].trimmed()) + '>');

                m_showModel_->clear();
                QHash<QString, QStandardItem *> roots{};
                const auto &changes = QString::fromUtf8(param[5]).split('\n', Qt::SkipEmptyParts);
                for (const auto &value: changes) {
                    const auto change = value.split('\t');
                    if (change.size() < 2) continue;
                    const auto &status = g_gitStatusCode[change[0].front()];
                    QString path1{};
                    QString path2{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        path1 = change[1];
                        path2 = change[2];
                    } else {
                        path2 = change[1];
                    }
                    const auto &path = path2.split('/');
                    const auto &documentPath = QDir(g_gitRootPath).filePath(path2);
                    const auto &documentUrl = QUrl::fromLocalFile(documentPath);
                    QString display{};
                    if (status == GitStatusCode::Renamed || status == GitStatusCode::Copied) {
                        display = QString("%1 -> %2 (%3%)").arg(
                            QUrl::fromLocalFile(QDir(g_gitRootPath).filePath(path1)).fileName(),
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
                gitEnabledSet();
                gitWatch();
            }
            break;
            case GitCommand::Add: {
                if (m_current.isEmpty()) {
                    gitWatch();
                    gitStatus();
                }
                if (m_gitStatus != GitStatus::Idle) {
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
            case GitCommand::Fetch: gitWatch();
            case GitCommand::Continue:
            case GitCommand::Abort:
            case GitCommand::Push: {
                emit removeBackground(m_taskId);
                gitStatusSet(GitStatus::Idle);
            }
            break;
            case GitCommand::Ahead: gitDiff_();
                break;
            case GitCommand::Watch: {
                emit updateIndex();
                gitRepo();
                gitRemoteGet();
            }
            break;
            case GitCommand::RemoteAdd: gitFetch();
                break;
            case GitCommand::RemoteGet:
            case GitCommand::UpstreamUnset:
            case GitCommand::UpstreamSet: gitUpstreamGet();
                break;
            case GitCommand::UpstreamGet: gitBranch();
                break;
            case GitCommand::Branch: gitLog();
                break;
            case GitCommand::Switch: gitUpstreamGet();
                break;
            case GitCommand::Create:
            case GitCommand::Rename:
            case GitCommand::Delete:
            case GitCommand::CherryPick: gitWatch();
                break;
            case GitCommand::Log: {
                if (m_logModel->rowCount() > 0) {
                    const auto hash = m_logModel->item(0, 0)->data(LogModel::HashRole).toString();
                    gitShowCommit(hash);
                }
            }
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
            case GitCommand::UpstreamGet: m_upstream = "";
                break;
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
                gitStatusSet(GitStatus::Idle);
            }
            break;
            case GitCommand::Push: {
                title = tr("Push Failed");
                text = QString::fromUtf8(error).trimmed();
                emit removeBackground(m_taskId);
                gitStatusSet(GitStatus::Idle);
            }
            break;
            default: {
                title = tr("Git command failed");
                text = QString::fromUtf8(error).trimmed();
            }
            break;
        }
        if (!title.isEmpty() && !text.isEmpty()) {
            m_errorDialog->setProperty("title", title);
            m_errorDialog->setProperty("text", text);
            QMetaObject::invokeMethod(m_errorDialog, "open");
        }
        // state machine
        switch (command) {
            case GitCommand::UpstreamGet: gitBranch();
                break;
            default: break;
        }
    }
    processDequeue();
}

// public
BranchModel::BranchModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &BranchModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &BranchModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &BranchModel::emptyChanged);
}

QHash<int, QByteArray> BranchModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[TypeRole] = "type";
    roles[HashRole] = "hash";
    roles[CommitRole] = "commit";
    return roles;
}

// public
QHash<int, QByteArray> LogModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[HashRole] = "hash";
    roles[PositionRole] = "pos";
    roles[ParentRole] = "parent";
    return roles;
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

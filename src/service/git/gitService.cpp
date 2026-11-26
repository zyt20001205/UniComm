#include "service/git/gitService.h"

#include <QDir>
#include <QDirListing>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <utility>

#include "globals.h"

// public
GitService::GitService(QObject *parent)
    : BaseService("Git", parent) {
    m_process.setWorkingDirectory(g_workspaceUrl.toLocalFile());
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    environment.insert("GIT_EDITOR", ":");
    environment.insert("GIT_OPTIONAL_LOCKS", "0");
    m_process.setProcessEnvironment(environment);
    connect(&m_process, &QProcess::finished, this, &GitService::processFinish);
    connect(&m_process, &QProcess::errorOccurred, this, &GitService::processError);
    m_timer.setSingleShot(true);
    m_timer.setInterval(30000);
    connect(&m_timer, &QTimer::timeout, &m_process, &QProcess::kill);

    m_watchTimer.setSingleShot(true);
    m_watchTimer.setInterval(100);
    const auto pathChanged = [this](const QString &path) {
        const auto refs = QDir(m_repo.commonDirPath).filePath("refs");
        m_changedPaths.insert(path.startsWith(refs + '/') ? refs : path);
        m_watchTimer.start();
    };
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, pathChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, pathChanged);
    connect(&m_watchTimer, &QTimer::timeout, this, &GitService::repoWatch);

    versionGet().then(this, [this](const GitResult &result) {
        if (result.success) repoRefresh();
    });
}

GitService::~GitService() {
    m_process.disconnect(this); // NOLINT
}

QFuture<GitResult> GitService::versionGet() {
    return processEnqueue({"--version"}).then(this, [this](const GitResult &result) {
        // update service state
        m_available = result.success;
        emit availableChanged();
        m_version = result.success ? result.text() : QString{};
        detailUpdate();
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

// public: operation
QFuture<GitResult> GitService::operationContinue(const GitOperationState::Value state, const QString &message) {
    QStringList arguments{};
    switch (state) {
        case GitOperationState::Merge:
            if (message.isEmpty()) arguments = {"merge", "--continue"};
            else arguments = {"commit", "-m", message};
            break;
        case GitOperationState::Rebase: arguments = {"rebase", "--continue"};
            break;
        case GitOperationState::CherryPick: arguments = {"cherry-pick", "--continue"};
            break;
        case GitOperationState::Revert: arguments = {"revert", "--continue"};
            break;
        default: return QtFuture::makeReadyValueFuture(GitResult{.raw = tr("No operation to continue.").toUtf8()});
    }
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::operationAbort(const GitOperationState::Value state) {
    QStringList arguments{};
    switch (state) {
        case GitOperationState::Merge: arguments = {"merge", "--abort"};
            break;
        case GitOperationState::Rebase: arguments = {"rebase", "--abort"};
            break;
        case GitOperationState::CherryPick: arguments = {"cherry-pick", "--abort"};
            break;
        case GitOperationState::Revert: arguments = {"revert", "--abort"};
            break;
        default: return QtFuture::makeReadyValueFuture(GitResult{.raw = tr("No operation to abort.").toUtf8()});
    }
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

// public: repo
QFuture<GitResult> GitService::init() {
    return processEnqueue({"init"}).then(this, [this](const GitResult &result) {
        // update service state
        if (result.success) repoRefresh();
        else statusSet(Status::Error);
        return result;
    });
}

QFuture<GitRepoResult> GitService::repoRefresh() {
    return processEnqueue({"rev-parse", "--path-format=absolute", "--show-toplevel", "--absolute-git-dir", "--git-common-dir", "--git-path", "index"}).then(
        this, [this](const GitResult &result) {
            const auto paths = result.text().replace("\r\n", "\n").split('\n');
            GitRepoResult repo{result};
            repo.success = result.success && paths.size() == 4;
            if (repo.success) {
                repo.workTreePath = paths[0];
                repo.gitDirPath = paths[1];
                repo.commonDirPath = paths[2];
                repo.indexPath = paths[3];
            }

            // update service state
            const bool hadRepo = hasRepoGet();
            m_repo = repo;
            statusSet(repo.success || (!result.success && result.raw.contains("not a git repository")) ? Status::Stopped : Status::Error);
            repoWatch();
            if (hasRepoGet() != hadRepo) emit hasRepoChanged();
            return repo;
        });
}

// public: remote
QFuture<GitResult> GitService::fetch() {
    return processEnqueue({"fetch", "--prune"});
}

QFuture<GitResult> GitService::push() {
    return processEnqueue({"push"});
}

void GitService::transferCancel() {
    if (!m_transfer) return;
    m_transfer->cancelled = true;
    if (m_transfer == &m_queue.front() && m_process.state() != QProcess::NotRunning) m_process.kill();
}

// public: branch
QFuture<GitResult> GitService::upstreamGet() {
    return processEnqueue({"rev-parse", "--symbolic-full-name", "@{upstream}"}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(Status::Stopped);
        return result;
    });
}

QFuture<GitResult> GitService::upstreamSet(const QString &upstream) {
    return processEnqueue({"branch", "--set-upstream-to=" + upstream}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::upstreamUnset() {
    return processEnqueue({"branch", "--unset-upstream"}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitRefsResult> GitService::refsGet() {
    return processEnqueue({"for-each-ref", "--format=%(HEAD)%00%(refname)%00%(symref)", "refs/heads", "refs/remotes", "refs/tags"})
            .then(this, [this](const GitResult &result) {
                GitRefsResult refs{result};
                if (result.success) {
                    for (const auto &line: result.raw.split('\n')) {
                        const auto fields = line.split('\0');
                        if (fields.size() == 3 && fields[2].trimmed().isEmpty())
                            refs.refs.append({QString::fromUtf8(fields[1]), fields[0] == "*"});
                    }
                }

                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return refs;
            });
}

QFuture<GitResult> GitService::branchCreate(const QString &startPoint, const QString &name, const bool checkout) {
    const QStringList arguments = checkout
                                      ? QStringList{"switch", "-c", name, "--", startPoint}
                                      : QStringList{"branch", "--", name, startPoint};
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::branchSwitch(const QString &name) {
    return processEnqueue({"switch", "--no-guess", "--", name}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::branchRename(const QString &oldName, const QString &newName) {
    return processEnqueue({"branch", "-m", "--", oldName, newName}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::branchDelete(const QString &name) {
    return processEnqueue({"branch", "-D", "--", name}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::merge(const QString &ref) {
    return processEnqueue({"merge", "--no-edit", ref}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::rebase(const QString &ref) {
    return processEnqueue({"rebase", ref}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

// public: history
QFuture<GitLogResult> GitService::logGet(const QString &ref) {
    return processEnqueue({"log", "-z", "--no-show-signature", "--format=tformat:%H%x00%P%x00%ar%x00%an%x00%s", ref, "--"})
            .then(this, [this](const GitResult &result) {
                GitLogResult log{result};
                if (result.success) {
                    const auto fields = result.raw.split('\0');
                    for (qsizetype i = 0; i + 4 < fields.size(); i += 5) {
                        log.commits.append({
                            .hash = QString::fromLatin1(fields[i]), .parents = QString::fromLatin1(fields[i + 1]).split(' ', Qt::SkipEmptyParts),
                            .relativeDate = QString::fromUtf8(fields[i + 2]), .author = QString::fromUtf8(fields[i + 3]), .subject = QString::fromUtf8(fields[i + 4])
                        });
                    }
                }

                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return log;
            });
}

QFuture<GitShowResult> GitService::showGet(const QString &hash) {
    return processEnqueue({
                "show", "--no-show-signature", "--diff-merges=first-parent", "--find-copies", "--no-relative",
                "--name-status", "-z", "--format=tformat:%h%x00%s%x00%ad%x00%an%x00%ae", "--end-of-options", hash + "^{commit}", "--"
            })
            .then(this, [this](const GitResult &result) {
                GitShowResult show{result};
                if (result.success) {
                    const auto fields = result.raw.split('\0');
                    show.shortHash = QString::fromLatin1(fields.at(0));
                    show.subject = QString::fromUtf8(fields.at(1));
                    show.date = QString::fromUtf8(fields.at(2));
                    show.author = QString::fromUtf8(fields.at(3));
                    show.email = QString::fromUtf8(fields.at(4));

                    const QDir workTree(m_repo.workTreePath);
                    for (qsizetype i = 5; i + 1 < fields.size();) {
                        // Git separates the header and the first status with a newline.
                        const auto code = fields.at(i++).trimmed();
                        GitShowResult::Entry file{.status = GitFileStatus::fromCode(code.front())};
                        if (file.status == GitFileStatus::Renamed || file.status == GitFileStatus::Copied) {
                            file.originalUrl = QUrl::fromLocalFile(workTree.filePath(QString::fromUtf8(fields.at(i++))));
                            file.similarity = code.sliced(1).toInt();
                        }
                        file.documentUrl = QUrl::fromLocalFile(workTree.filePath(QString::fromUtf8(fields.at(i++))));
                        show.files.append(file);
                    }
                }

                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return show;
            });
}

QFuture<GitResult> GitService::cherryPick(const QString &hash) {
    return processEnqueue({"cherry-pick", hash}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::revert(const QString &hash) {
    return processEnqueue({"revert", "--no-edit", hash}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::reset(const QString &hash, const GitResetMode::Value mode) {
    QString option{};
    switch (mode) {
        case GitResetMode::Mixed: option = "--mixed";
            break;
        case GitResetMode::Soft: option = "--soft";
            break;
        case GitResetMode::Hard: option = "--hard";
            break;
        case GitResetMode::Merge: option = "--merge";
            break;
        case GitResetMode::Keep: option = "--keep";
            break;
        default: return QtFuture::makeReadyValueFuture(GitResult{.raw = tr("Unknown reset mode.").toUtf8()});
    }
    return processEnqueue({"reset", option, hash, "--"}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

// public: working tree
QFuture<GitStatusResult> GitService::statusGet(const bool includeIgnored) {
    QStringList arguments{"status", "--porcelain=v1", "-z", "-uall"};
    if (includeIgnored) arguments.append("--ignored");
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        GitStatusResult status{result};
        if (result.success) {
            const QDir workTree(m_repo.workTreePath);
            const auto records = result.raw.split('\0');
            for (qsizetype i = 0; i < records.size(); ++i) {
                const auto &record = records.at(i);
                if (record.isEmpty()) continue;
                const auto indexStatus = GitFileStatus::fromCode(record.at(0));
                const auto workingTreeStatus = GitFileStatus::fromCode(record.at(1));
                const auto path = QDir::cleanPath(workTree.filePath(QString::fromUtf8(record.sliced(3))));
                status.files.append({QUrl::fromLocalFile(path), indexStatus, workingTreeStatus});
                // renames and copies carry the original path in the next record
                if (indexStatus == GitFileStatus::Renamed || indexStatus == GitFileStatus::Copied ||
                    workingTreeStatus == GitFileStatus::Renamed || workingTreeStatus == GitFileStatus::Copied)
                    ++i;
            }
        }

        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return status;
    });
}

QFuture<GitConflictResult> GitService::conflictGet(const QUrl &url) {
    return processEnqueue({"--literal-pathspecs", "ls-files", "--unmerged", "-z", "--", url.toLocalFile()})
            .then(this, [this](const GitResult &result) {
                if (!result.success || result.raw.isEmpty()) return QtFuture::makeReadyValueFuture(GitConflictResult{result});

                QList<int> stages{};
                QList<QFuture<GitResult> > blobs{};
                for (const auto &record: result.raw.split('\0')) {
                    if (record.isEmpty()) continue;
                    // Each record starts with mode, object ID and stage, followed by a tab and the path.
                    const auto fields = record.first(record.indexOf('\t')).split(' ');
                    stages.append(fields.at(2).toInt());
                    blobs.append(processEnqueue({"cat-file", "blob", QString::fromLatin1(fields.at(1))}));
                }
                return QtFuture::whenAll(blobs.begin(), blobs.end()).then([stages](const QList<QFuture<GitResult> > &finished) {
                    GitConflictResult conflict{};
                    for (qsizetype i = 0; i < finished.size(); ++i) {
                        const auto blob = finished.at(i).result();
                        if (!blob.success) return GitConflictResult{blob};
                        switch (stages.at(i)) {
                            case 1: conflict.base = blob.raw;
                                break;
                            case 2: conflict.ours = blob.raw;
                                break;
                            case 3: conflict.theirs = blob.raw;
                                break;
                            default: ;
                        }
                    }
                    conflict.success = true;
                    return conflict;
                });
            }).unwrap().then(this, [this](const GitConflictResult &result) {
                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return result;
            });
}

QFuture<GitStagedDiffResult> GitService::diffGet() {
    return processEnqueue({"diff", "--cached", "--name-status", "--no-renames", "--no-relative", "-z"}).then(this, [this](const GitResult &result) {
        GitStagedDiffResult diff{result};
        if (result.success) {
            diff.hasChanges = !result.raw.isEmpty();
            const QDir workTree(m_repo.workTreePath);
            const auto fields = result.raw.split('\0');
            for (qsizetype i = 0; i + 1 < fields.size(); i += 2) {
                if (fields.at(i) == "U") diff.conflicts.append(QUrl::fromLocalFile(workTree.filePath(QString::fromUtf8(fields.at(i + 1)))));
            }
        }

        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return diff;
    });
}

QFuture<GitDiffResult> GitService::diffGet(const QByteArray &left, const QByteArray &right) {
    const auto directory = QSharedPointer<QTemporaryDir>::create();
    if (!directory->isValid()) return QtFuture::makeReadyValueFuture(GitDiffResult{GitResult{.raw = directory->errorString().toUtf8()}});

    const auto leftPath = directory->filePath("left");
    const auto rightPath = directory->filePath("right");
    for (const auto &[path, contents]: {std::pair{leftPath, left}, std::pair{rightPath, right}}) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.flush())
            return QtFuture::makeReadyValueFuture(GitDiffResult{GitResult{.raw = file.errorString().toUtf8()}});
    }

    // Keep both input files until the queued diff finishes.
    return processEnqueue({
                              "diff", "--no-index", "--text", "--no-color", "--no-ext-diff", "--no-textconv", "--unified=0", "--inter-hunk-context=0", "--", leftPath, rightPath
                          }, {0, 1})
            .then(this, [this, directory](const GitResult &result) {
                GitDiffResult diff{result};
                if (result.success) {
                    static const QRegularExpression header(QStringLiteral(R"(^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@)"), QRegularExpression::MultilineOption);
                    auto matches = header.globalMatch(QString::fromLatin1(result.raw));
                    while (matches.hasNext()) {
                        const auto match = matches.next();
                        const int leftCount = match.hasCaptured(2) ? match.capturedView(2).toInt() : 1;
                        const int rightCount = match.hasCaptured(4) ? match.capturedView(4).toInt() : 1;
                        // An empty range already points at the zero-based insertion position.
                        diff.hunks.append({
                            .leftStart = match.capturedView(1).toInt() - (leftCount > 0), .leftCount = leftCount,
                            .rightStart = match.capturedView(3).toInt() - (rightCount > 0), .rightCount = rightCount
                        });
                    }
                }

                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return diff;
            });
}

QFuture<GitResult> GitService::add(const QUrl &url) {
    const QStringList arguments = url.isEmpty()
                                      ? QStringList{"add", "--all"}
                                      : QStringList{"--literal-pathspecs", "add", "--", url.toLocalFile()};
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::untrack(const QUrl &url) {
    return processEnqueue({"--literal-pathspecs", "rm", "-r", "--cached", "--ignore-unmatch", "--", url.toLocalFile()}).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

QFuture<GitResult> GitService::restore(const QUrl &url, const GitRestoreMode::Value mode) {
    QStringList arguments{"restore"};
    switch (mode) {
        case GitRestoreMode::Worktree:
            arguments << "--worktree";
            break;
        case GitRestoreMode::Staged:
            arguments << "--staged";
            break;
        case GitRestoreMode::Both:
            arguments << "--worktree" << "--staged";
            break;
        default:
            return QtFuture::makeReadyValueFuture(GitResult{.raw = tr("Unknown restore mode.").toUtf8()});
    }
    if (url.isEmpty()) arguments << "--" << ":/";
    else {
        arguments.prepend("--literal-pathspecs");
        arguments << "--" << url.toLocalFile();
    }
    return processEnqueue(arguments).then(this, [this](const GitResult &result) {
        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        if (result.success) emit indexChanged();
        return result;
    });
}

QFuture<GitResult> GitService::ignore(const QUrl &url) {
    const auto result = ignoreSet(url, true);
    if (!result.success) return QtFuture::makeReadyValueFuture(result);
    return untrack(url);
}

QFuture<GitResult> GitService::unignore(const QUrl &url) {
    return QtFuture::makeReadyValueFuture(ignoreSet(url, false));
}

// private
void GitService::detailUpdate() {
    if (!m_available) {
        detailSet({});
        return;
    }
    const auto repo = hasRepoGet()
                          ? QString("<a href=\"%1\">%2</a>").arg(QUrl::fromLocalFile(m_repo.workTreePath).toString(QUrl::FullyEncoded).toHtmlEscaped(),
                                                                 m_repo.workTreePath.toHtmlEscaped())
                          : tr("None");
    const auto files = m_watcher.files().size();
    const auto directories = m_watcher.directories().size();
    const auto watcher = files || directories ? tr("Watching · %1 files, %2 directories").arg(files).arg(directories) : tr("Inactive");
    detailSet(QString("<b>%1</b><br>%2<br><b>%3</b><br>%4<br><b>%5</b><br>%6")
        .arg(tr("Version:"), m_version.toHtmlEscaped(), tr("Repository:"), repo, tr("Watcher:"), watcher));
}

void GitService::repoWatch() {
    QSet<QString> paths{};
    const QDir gitDir(m_repo.gitDirPath);
    const QDir commonDir(m_repo.commonDirPath);
    const auto head = gitDir.filePath("HEAD");
    const auto refs = commonDir.filePath("refs");
    const auto packedRefs = commonDir.filePath("packed-refs");
    const auto config = commonDir.filePath("config");
    const QSet operationFiles{
        gitDir.filePath("MERGE_HEAD"), gitDir.filePath("REBASE_HEAD"), gitDir.filePath("rebase-merge/head-name"),
        gitDir.filePath("rebase-apply/rebasing"), gitDir.filePath("CHERRY_PICK_HEAD"), gitDir.filePath("REVERT_HEAD")
    };
    if (hasRepoGet()) {
        if (!gitDir.exists() || !commonDir.exists()) {
            repoRefresh();
            return;
        }
        paths = {m_repo.gitDirPath, m_repo.commonDirPath, QFileInfo(m_repo.indexPath).absolutePath()};
        for (const auto &path: operationFiles | QSet{m_repo.indexPath, head, packedRefs, config, refs}) {
            if (QFileInfo::exists(path)) paths.insert(path);
        }
        for (const auto &entry: QDirListing(refs, QDirListing::IteratorFlag::DirsOnly | QDirListing::IteratorFlag::Recursive)) {
            paths.insert(entry.filePath());
        }
    }

    // Git replaces files through lockfiles. Reattach paths that Qt dropped.
    const auto watched = m_watcher.files() + m_watcher.directories();
    const QSet registered(watched.begin(), watched.end());
    const auto removed = registered - paths;
    const auto added = paths - registered;
    if (!removed.isEmpty()) m_watcher.removePaths(removed.values());
    if (!added.isEmpty()) m_watcher.addPaths(added.values());
    detailUpdate();

    const auto changed = std::exchange(m_changedPaths, {}) | added;
    if (!hasRepoGet()) return;

    if (!registered.contains(m_repo.gitDirPath)) {
        emit gitDirChanged();
        emit refsChanged();
        emit indexChanged();
        return;
    }

    if (changed.intersects(operationFiles)) emit gitDirChanged();
    if (changed.contains(head) || changed.contains(packedRefs) || changed.contains(config) || changed.contains(refs)) emit refsChanged();
    if (changed.contains(m_repo.indexPath)) emit indexChanged();
}

GitResult GitService::ignoreSet(const QUrl &url, const bool ignored) {
    const QDir workTree(m_repo.workTreePath);
    auto rule = '/' + workTree.relativeFilePath(url.toLocalFile()).toUtf8();
    rule.replace("\\", "\\\\").replace("*", "\\*").replace("?", "\\?").replace("[", "\\[");
    if (rule.endsWith(' ')) rule.insert(rule.size() - 1, '\\');

    QFile file(workTree.filePath(".gitignore"));
    QByteArray contents{};
    QByteArray newline = "\n";
    bool found = false;
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) return {.raw = file.errorString().toUtf8()};
        if (file.peek(3) == "\xef\xbb\xbf") contents = file.read(3);
        while (!file.atEnd()) {
            const auto line = file.readLine();
            if (line.endsWith('\n')) newline = line.endsWith("\r\n") ? "\r\n" : "\n";
            if (line == rule || line == rule + newline) {
                found = true;
                if (!ignored) continue;
            }
            contents.append(line);
        }
        if (file.error() != QFileDevice::NoError) return {.raw = file.errorString().toUtf8()};
        file.close();
    }
    if (found == ignored) return {.success = true};
    if (ignored) {
        if (!contents.isEmpty() && !contents.endsWith('\n')) contents.append(newline);
        contents.append(rule).append(newline);
    }

    QSaveFile output(file.fileName());
    if (!output.open(QIODevice::WriteOnly) || output.write(contents) != contents.size() || !output.commit())
        return {.raw = output.errorString().toUtf8()};
    emit indexChanged();
    return {.success = true};
}

QFuture<GitResult> GitService::processEnqueue(const QStringList &arguments, QList<int> successCodes) {
    const bool transfer = arguments.first() == "fetch" || arguments.first() == "push";
    if (transfer && m_transfer) return QtFuture::makeReadyValueFuture(GitResult{.raw = tr("A transfer is already in progress.").toUtf8()});
    auto &command = m_queue.emplace(arguments, std::move(successCodes));
    if (transfer) m_transfer = &command;
    command.promise.start();
    auto future = command.promise.future();
    if (m_queue.size() == 1) processStart();
    return future;
}

void GitService::processStart() {
    const auto &command = m_queue.front();
    statusSet(Status::Running);
    if (command.cancelled) {
        processFinish();
        return;
    }
    if (&command != m_transfer) m_timer.start();
    m_process.start("git", command.arguments);
}

void GitService::processError(const QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) processFinish();
}

void GitService::processFinish() {
    m_timer.stop();
    const bool transfer = &m_queue.front() == m_transfer;
    auto [arguments, successCodes, promise, cancelled] = std::move(m_queue.front());
    GitResult result{};
    if (!cancelled) {
        result.success = m_process.error() != QProcess::FailedToStart && m_process.exitStatus() == QProcess::NormalExit && successCodes.contains(m_process.exitCode());
        const auto output = m_process.readAllStandardOutput();
        const auto error = m_process.readAllStandardError();
        result.raw = result.success || error.isEmpty() ? output : error;
        if (!result.success && result.raw.isEmpty() && m_process.error() != QProcess::UnknownError) result.raw = m_process.errorString().toUtf8();
    }
    if (transfer) {
        m_transfer = nullptr;
        statusSet(cancelled || result.success ? Status::Stopped : Status::Error);
    }
    const auto &event = arguments.first() == "--literal-pathspecs" ? arguments.at(1) : arguments.first();
    if (cancelled) eventAppend(event, false, tr("Cancelled"));
    else eventAppend(event, result.success, result.success ? "git " + arguments.join(' ') : result.text());
    m_queue.pop();
    const bool hasNext = !m_queue.empty();
    if (cancelled) promise.future().cancel();
    else promise.addResult(std::move(result));
    promise.finish();
    if (hasNext) processStart();
}

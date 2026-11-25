#include "service/git/gitService.h"

#include <QDir>
#include <QDirListing>
#include <QFile>
#include <QSaveFile>
#include <utility>

#include "globals.h"

// public
GitService::GitService(QObject *parent)
    : BaseService("Git", parent) {
    m_process.setWorkingDirectory(g_workspaceUrl.toLocalFile());
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    environment.insert("GIT_OPTIONAL_LOCKS", "0");
    m_process.setProcessEnvironment(environment);
    connect(&m_process, &QProcess::finished, this, &GitService::processFinish);
    connect(&m_process, &QProcess::errorOccurred, this, &GitService::processError);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(30000);
    connect(&m_timeout, &QTimer::timeout, &m_process, &QProcess::kill);

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
        m_version = result.success ? result.output : QString{};
        detailUpdate();
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
            const auto paths = QString(result.output).replace("\r\n", "\n").split('\n');
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
            statusSet(repo.success || (!result.success && result.output.contains("not a git repository")) ? Status::Stopped : Status::Error);
            repoWatch();
            if (hasRepoGet() != hadRepo) emit hasRepoChanged();
            return repo;
        });
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
                    for (const auto &line: result.output.split('\n', Qt::SkipEmptyParts)) {
                        const auto fields = line.split(QChar::Null);
                        if (fields.size() == 3 && fields[2].isEmpty())
                            refs.refs.append({fields[1], fields[0] == "*"});
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

// public: history
QFuture<GitLogResult> GitService::logGet(const QString &ref) {
    return processEnqueue({"log", "-z", "--no-show-signature", "--format=tformat:%H%x00%P%x00%ar%x00%an%x00%s", ref, "--"})
            .then(this, [this](const GitResult &result) {
                GitLogResult log{result};
                if (result.success) {
                    const auto fields = result.output.split(QChar::Null);
                    for (qsizetype i = 0; i + 4 < fields.size(); i += 5) {
                        log.commits.append({
                            .hash = fields[i], .parents = fields[i + 1].split(' ', Qt::SkipEmptyParts),
                            .relativeDate = fields[i + 2], .author = fields[i + 3], .subject = fields[i + 4]
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
                    const auto fields = result.output.split(QChar::Null);
                    show.shortHash = fields.at(0);
                    show.subject = fields.at(1);
                    show.date = fields.at(2);
                    show.author = fields.at(3);
                    show.email = fields.at(4);

                    const QDir workTree(m_repo.workTreePath);
                    for (qsizetype i = 5; i + 1 < fields.size();) {
                        // Git separates the header and the first status with a newline.
                        const auto code = fields.at(i++).trimmed();
                        GitShowResult::Entry file{.status = GitFileStatus::fromCode(code.front())};
                        if (file.status == GitFileStatus::Renamed || file.status == GitFileStatus::Copied) {
                            file.originalUrl = QUrl::fromLocalFile(workTree.filePath(fields.at(i++)));
                            file.similarity = code.sliced(1).toInt();
                        }
                        file.documentUrl = QUrl::fromLocalFile(workTree.filePath(fields.at(i++)));
                        show.files.append(file);
                    }
                }

                // update service state
                statusSet(result.success ? Status::Stopped : Status::Error);
                return show;
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
            const auto records = result.output.split(QChar::Null, Qt::SkipEmptyParts);
            for (qsizetype i = 0; i < records.size(); ++i) {
                const auto &record = records.at(i);
                const auto indexStatus = GitFileStatus::fromCode(record.at(0));
                const auto workingTreeStatus = GitFileStatus::fromCode(record.at(1));
                const auto path = QDir::cleanPath(workTree.filePath(record.sliced(3)));
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
    const QSet operationFiles{gitDir.filePath("MERGE_HEAD"), gitDir.filePath("REBASE_HEAD"), gitDir.filePath("CHERRY_PICK_HEAD")};
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
        if (!file.open(QIODevice::ReadOnly)) return {.output = file.errorString()};
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
        if (file.error() != QFileDevice::NoError) return {.output = file.errorString()};
        file.close();
    }
    if (found == ignored) return {.success = true};
    if (ignored) {
        if (!contents.isEmpty() && !contents.endsWith('\n')) contents.append(newline);
        contents.append(rule).append(newline);
    }

    QSaveFile output(file.fileName());
    if (!output.open(QIODevice::WriteOnly) || output.write(contents) != contents.size() || !output.commit())
        return {.output = output.errorString()};
    emit indexChanged();
    return {.success = true};
}

QFuture<GitResult> GitService::processEnqueue(const QStringList &arguments) {
    auto &command = m_queue.emplace(arguments);
    command.promise.start();
    auto future = command.promise.future();
    if (m_queue.size() == 1) processStart();
    return future;
}

void GitService::processStart() {
    statusSet(Status::Running);
    m_timeout.start();
    m_process.start("git", m_queue.front().arguments);
}

void GitService::processError(const QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) processFinish();
}

void GitService::processFinish() {
    m_timeout.stop();
    const bool success = m_process.error() != QProcess::FailedToStart && m_process.exitStatus() == QProcess::NormalExit && m_process.exitCode() == 0;
    auto output = QString::fromUtf8(success ? m_process.readAllStandardOutput() : m_process.readAllStandardError());
    if (output.endsWith("\r\n")) output.chop(2);
    else if (output.endsWith('\n')) output.chop(1);
    if (!success && output.isEmpty() && m_process.error() != QProcess::UnknownError) output = m_process.errorString();

    // record command
    auto [arguments, promise] = std::move(m_queue.front());
    const auto &event = arguments.first() == "--literal-pathspecs" ? arguments.at(1) : arguments.first();
    eventAppend(event, success, success ? "git " + arguments.join(' ') : output);
    m_queue.pop();
    const bool hasNext = !m_queue.empty();
    promise.addResult(GitResult{.success = success, .output = output});
    promise.finish();
    if (hasNext) processStart();
}

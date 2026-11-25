#include "service/git/gitService.h"

#include <QDir>

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
    m_timeout.setInterval(std::chrono::seconds(30));
    connect(&m_timeout, &QTimer::timeout, &m_process, &QProcess::kill);

    versionGet().then(this, [this](const GitResult &result) {
        if (result.success) repositoryGet();
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
        detailSet(result.success ? result.output.toHtmlEscaped().replace('\n', "<br>") : QString{});
        statusSet(result.success ? Status::Stopped : Status::Error);
        return result;
    });
}

// public: repository
QFuture<GitRepositoryResult> GitService::repositoryGet() {
    return processEnqueue({"rev-parse", "--show-toplevel", "--absolute-git-dir"}).then(this, [this](const GitResult &result) {
        const auto paths = QString(result.output).replace("\r\n", "\n").split('\n');
        GitRepositoryResult repository{result};
        repository.success = result.success && paths.size() == 2;
        if (repository.success) {
            repository.rootPath = paths.first();
            repository.gitDirPath = paths.last();
        }

        // update service state
        const bool hadRepository = hasRepositoryGet();
        m_rootPath = repository.rootPath;
        m_gitDirPath = repository.gitDirPath;
        if (hasRepositoryGet() != hadRepository) emit hasRepositoryChanged();
        statusSet(repository.success || (!result.success && result.output.contains("not a git repository")) ? Status::Stopped : Status::Error);
        return repository;
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

QFuture<GitRefsResult> GitService::refsGet() {
    return processEnqueue({"for-each-ref", "--format=%(HEAD)%00%(refname)%00%(symref)", "refs/heads", "refs/remotes", "refs/tags"})
        .then(this, [this](const GitResult &result) {
            GitRefsResult refs{result};
            if (result.success) {
                for (const auto &line : result.output.split('\n', Qt::SkipEmptyParts)) {
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
    const QStringList arguments = checkout ? QStringList{"switch", "-c", name, "--", startPoint}
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
                    log.commits.append({fields[i], fields[i + 1].split(' ', Qt::SkipEmptyParts),
                                        fields[i + 2], fields[i + 3], fields[i + 4]});
                }
            }

            // update service state
            statusSet(result.success ? Status::Stopped : Status::Error);
            return log;
        });
}

QFuture<GitShowResult> GitService::showGet(const QString &hash) {
    return processEnqueue({"show", "--no-show-signature", "--diff-merges=first-parent", "--find-copies", "--no-relative",
                           "--name-status", "-z", "--format=tformat:%h%x00%s%x00%ad%x00%an%x00%ae", "--end-of-options", hash + "^{commit}", "--"})
        .then(this, [this](const GitResult &result) {
            GitShowResult show{result};
            if (result.success) {
                const auto fields = result.output.split(QChar::Null);
                show.shortHash = fields.at(0);
                show.subject = fields.at(1);
                show.date = fields.at(2);
                show.author = fields.at(3);
                show.email = fields.at(4);

                const QDir root(m_rootPath);
                for (qsizetype i = 5; i + 1 < fields.size();) {
                    // Git separates the header and the first status with a newline.
                    const auto code = fields.at(i++).trimmed();
                    GitShowResult::Entry file{.status = GitFileStatus::fromCode(code.front())};
                    if (file.status == GitFileStatus::Renamed || file.status == GitFileStatus::Copied) {
                        file.originalUrl = QUrl::fromLocalFile(root.filePath(fields.at(i++)));
                        file.similarity = code.sliced(1).toInt();
                    }
                    file.documentUrl = QUrl::fromLocalFile(root.filePath(fields.at(i++)));
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
            const QDir root(m_rootPath);
            const auto records = result.output.split(QChar::Null, Qt::SkipEmptyParts);
            for (qsizetype i = 0; i < records.size(); ++i) {
                const auto &record = records.at(i);
                const auto indexStatus = GitFileStatus::fromCode(record.at(0));
                const auto workingTreeStatus = GitFileStatus::fromCode(record.at(1));
                const auto path = QDir::cleanPath(root.filePath(record.sliced(3)));
                status.files.insert(QUrl::fromLocalFile(path), {.indexStatus = indexStatus, .workingTreeStatus = workingTreeStatus});
                // renames and copies carry the original path in the next record
                if (indexStatus == GitFileStatus::Renamed || indexStatus == GitFileStatus::Copied ||
                    workingTreeStatus == GitFileStatus::Renamed || workingTreeStatus == GitFileStatus::Copied) ++i;
            }
        }

        // update service state
        statusSet(result.success ? Status::Stopped : Status::Error);
        return status;
    });
}

// private
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
    eventAppend(arguments.first(), success, success ? "git " + arguments.join(' ') : output);
    m_queue.pop();
    const bool hasNext = !m_queue.empty();
    promise.addResult(GitResult{.success = success, .output = output});
    promise.finish();
    if (hasNext) processStart();
}

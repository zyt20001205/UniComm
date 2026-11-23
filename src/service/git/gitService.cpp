#include "service/git/gitService.h"

#include "globals.h"

// public
GitService::GitService(QObject *parent)
    : BaseService("Git", parent) {
    m_process.setWorkingDirectory(g_workspaceUrl.toLocalFile());
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
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

QFuture<GitRepositoryResult> GitService::repositoryGet() {
    return processEnqueue({"rev-parse", "--show-toplevel", "--absolute-git-dir"}).then(this, [this](const GitResult &result) {
        const auto paths = result.output.split('\n');
        GitRepositoryResult repository{result};
        repository.success = result.success && paths.size() == 2;
        if (repository.success) {
            repository.rootPath = paths.first().trimmed();
            repository.gitDirPath = paths.last().trimmed();
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
    auto output = QString::fromUtf8(success ? m_process.readAllStandardOutput() : m_process.readAllStandardError()).trimmed();
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

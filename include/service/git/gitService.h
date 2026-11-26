#ifndef UNICOMM_GITSERVICE_H
#define UNICOMM_GITSERVICE_H

#include <optional>
#include <queue>
#include <QFileSystemWatcher>
#include <QFuture>
#include <QProcess>
#include <QPromise>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "service/baseService.h"
#include "types/gitTypes.h"

struct GitResult {
    bool success{};
    QByteArray raw{};

    [[nodiscard]] QString text() const {
        auto text = QString::fromUtf8(raw);
        if (text.endsWith("\r\n")) text.chop(2);
        else if (text.endsWith('\n')) text.chop(1);
        return text;
    }
};

struct GitRepoResult : GitResult {
    QString workTreePath{};
    QString gitDirPath{};
    QString commonDirPath{};
    QString indexPath{};
};

struct GitRefsResult : GitResult {
    struct Entry {
        QString ref;
        bool current{};
    };

    QList<Entry> refs{};
};

struct GitLogResult : GitResult {
    struct Entry {
        QString hash;
        QStringList parents;
        QString relativeDate;
        QString author;
        QString subject;
    };

    QList<Entry> commits{};
};

struct GitShowResult : GitResult {
    struct Entry {
        GitFileStatus::Value status;
        QUrl documentUrl;
        QUrl originalUrl;
        int similarity{};
    };

    QString shortHash{};
    QString subject{};
    QString date{};
    QString author{};
    QString email{};
    QList<Entry> files{};
};

struct GitStatusResult : GitResult {
    struct Entry {
        QUrl documentUrl;
        GitFileStatus::Value indexStatus;
        GitFileStatus::Value workingTreeStatus;
    };

    QList<Entry> files{};
};

struct GitConflictResult : GitResult {
    std::optional<QByteArray> base{};
    std::optional<QByteArray> ours{};
    std::optional<QByteArray> theirs{};
};

struct GitDiffResult : GitResult {
    struct Hunk {
        int leftStart{};
        int leftCount{};
        int rightStart{};
        int rightCount{};
    };

    QList<Hunk> hunks{};
};

class GitService : public BaseService {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool available READ availableGet NOTIFY availableChanged)
    Q_PROPERTY(bool hasRepo READ hasRepoGet NOTIFY hasRepoChanged)

public:
    explicit GitService(QObject *parent = nullptr);

    ~GitService() override;

    [[nodiscard]] bool availableGet() const {
        return m_available;
    }

    [[nodiscard]] bool hasRepoGet() const {
        return m_repo.success;
    }

    QFuture<GitResult> versionGet();

    // public: repo
    QFuture<GitResult> init();

    [[nodiscard]] const GitRepoResult &repoGet() const {
        return m_repo;
    }

    QFuture<GitRepoResult> repoRefresh();

    // public: branch
    QFuture<GitResult> upstreamGet();

    QFuture<GitResult> upstreamSet(const QString &upstream);

    QFuture<GitResult> upstreamUnset();

    QFuture<GitRefsResult> refsGet();

    QFuture<GitResult> branchCreate(const QString &startPoint, const QString &name, bool checkout);

    QFuture<GitResult> branchSwitch(const QString &name);

    QFuture<GitResult> branchRename(const QString &oldName, const QString &newName);

    QFuture<GitResult> branchDelete(const QString &name);

    QFuture<GitResult> merge(const QString &ref);

    QFuture<GitResult> rebase(const QString &ref);

    // public: history
    QFuture<GitLogResult> logGet(const QString &ref);

    QFuture<GitShowResult> showGet(const QString &hash);

    QFuture<GitResult> cherryPick(const QString &hash);

    QFuture<GitResult> revert(const QString &hash);

    QFuture<GitResult> reset(const QString &hash, GitResetMode::Value mode);

    // public: working tree
    QFuture<GitStatusResult> statusGet(bool includeIgnored = false);

    QFuture<GitConflictResult> conflictGet(const QUrl &url);

    QFuture<GitDiffResult> diffGet(const QByteArray &left, const QByteArray &right);

    QFuture<GitResult> add(const QUrl &url = {});

    QFuture<GitResult> untrack(const QUrl &url);

    QFuture<GitResult> restore(const QUrl &url, GitRestoreMode::Value mode);

    QFuture<GitResult> ignore(const QUrl &url);

    QFuture<GitResult> unignore(const QUrl &url);

signals:
    void availableChanged();

    void hasRepoChanged();

    void gitDirChanged();

    void refsChanged();

    void indexChanged();

private:
    void detailUpdate();

    void repoWatch();

    GitResult ignoreSet(const QUrl &url, bool ignored);

    QFuture<GitResult> processEnqueue(const QStringList &arguments, QList<int> successCodes = {0});

    void processStart();

    void processError(QProcess::ProcessError error);

    void processFinish();

    struct Command {
        QStringList arguments;
        QList<int> successCodes;
        QPromise<GitResult> promise{};
    };

    std::queue<Command> m_queue{};
    QProcess m_process{};
    QTimer m_timeout{};

    bool m_available{};
    QString m_version{};
    GitRepoResult m_repo{};

    QFileSystemWatcher m_watcher{};
    QTimer m_watchTimer{};
    QSet<QString> m_changedPaths{};
};

#endif //UNICOMM_GITSERVICE_H

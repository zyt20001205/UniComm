#ifndef UNICOMM_GITSERVICE_H
#define UNICOMM_GITSERVICE_H

#include <queue>
#include <QFuture>
#include <QProcess>
#include <QPromise>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "service/baseService.h"
#include "types/gitTypes.h"

struct GitResult {
    bool success{};
    QString output{};
};

struct GitRepositoryResult : GitResult {
    QString rootPath{};
    QString gitDirPath{};
};

struct GitRefsResult : GitResult {
    struct Entry {
        QString ref;
        bool current{};
    };

    QList<Entry> refs{};
};

struct GitStatusResult : GitResult {
    struct Entry {
        GitFileStatus::Value indexStatus;
        GitFileStatus::Value workingTreeStatus;
    };

    QHash<QUrl, Entry> files{};
};

class GitService : public BaseService {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool available READ availableGet NOTIFY availableChanged)
    Q_PROPERTY(bool hasRepository READ hasRepositoryGet NOTIFY hasRepositoryChanged)

public:
    explicit GitService(QObject *parent = nullptr);

    ~GitService() override;

    [[nodiscard]] bool availableGet() const {
        return m_available;
    }

    [[nodiscard]] bool hasRepositoryGet() const {
        return !m_rootPath.isEmpty();
    }

    [[nodiscard]] const QString &rootPathGet() const {
        return m_rootPath;
    }

    [[nodiscard]] const QString &gitDirPathGet() const {
        return m_gitDirPath;
    }

    QFuture<GitResult> versionGet();

    // public: repository
    QFuture<GitRepositoryResult> repositoryGet();

    // public: branch
    QFuture<GitResult> upstreamGet();

    QFuture<GitRefsResult> refsGet();

    // public: working tree
    QFuture<GitStatusResult> statusGet(bool includeIgnored = false);

signals:
    void availableChanged();

    void hasRepositoryChanged();

private:
    QFuture<GitResult> processEnqueue(const QStringList &arguments);

    void processStart();

    void processError(QProcess::ProcessError error);

    void processFinish();

    struct Command {
        QStringList arguments;
        QPromise<GitResult> promise{};
    };

    std::queue<Command> m_queue{};
    QProcess m_process{};
    QTimer m_timeout{};

    bool m_available{};
    QString m_rootPath{};
    QString m_gitDirPath{};
};

#endif //UNICOMM_GITSERVICE_H

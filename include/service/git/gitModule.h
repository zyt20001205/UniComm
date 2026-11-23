#ifndef UNICOMM_GITMODULE_H
#define UNICOMM_GITMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QJsonObject>
#include <QQueue>
#include <QStandardItemModel>

#include "types/gitTypes.h"

class QFileSystemWatcher;
class QProcess;
class QQuickView;
class QQuickWidget;
class QTextDocument;

class GitConfig;
class GitService;
class BranchModel;
class LogModel;
class ShowModel;
class StatusModel;
class CommitModel;

class GitModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(GitOperationState::Value gitStatus READ gitStatusGet NOTIFY gitStatusChanged)

public:
    explicit GitModule();

    ~GitModule() override;

    [[nodiscard]] GitOperationState::Value gitStatusGet() const {
        return m_gitStatus;
    }

    void gitStatusSet(const GitOperationState::Value state) {
        m_gitStatus = state;
        emit gitStatusChanged();
    }

    void propertySet(const QVariantHash &objects);

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    Q_INVOKABLE void propertyGet_(const QVariantMap &objects);

    Q_INVOKABLE void branchSet(const QString &name);

    Q_INVOKABLE void gitInit();

    Q_INVOKABLE void gitCommitPre();

    void gitStatus();

    Q_INVOKABLE void gitCommit(const QString &subject);

    Q_INVOKABLE void gitFetch();

    Q_INVOKABLE void gitPushPre();

    void gitAhead();

    Q_INVOKABLE void gitDiff_();

    Q_INVOKABLE void gitShowCommit_(const QString &hash);

    Q_INVOKABLE void gitPush();

    void gitWatch();

    void gitRepo();

    Q_INVOKABLE void gitRemoteAdd(const QString &upstreamUrl);

    void gitRemoteGet();

    Q_INVOKABLE void gitUpstreamSet(const QString &upstream);

    Q_INVOKABLE void gitUpstreamUnset();

    void gitUpstreamGet();

    Q_INVOKABLE void gitBranch();

    Q_INVOKABLE void gitSwitch(const QString &name);

    Q_INVOKABLE void gitCreate(const QString &src, const QString &dst, bool _switch);

    Q_INVOKABLE void gitRename(const QString &src, const QString &dst);

    Q_INVOKABLE void gitDelete(const QString &name);

    Q_INVOKABLE void gitLog();

    Q_INVOKABLE void gitReset(const QString &hash, GitResetMode::Value mode);

    Q_INVOKABLE void gitCherryPick(const QString &hash);

    Q_INVOKABLE void gitShowCommit(const QString &hash);

    Q_INVOKABLE void gitShowFile(const QString &hash, const QUrl &documentUrl);

    Q_INVOKABLE void gitMerge(const QString &name);

    Q_INVOKABLE void gitRebase(const QString &name);

    Q_INVOKABLE void gitAbort();

    Q_INVOKABLE void gitContinue(const QString &message);

    Q_INVOKABLE void gitDiff();

    Q_INVOKABLE void gitAdd(const QUrl &documentUrl = QUrl());

    Q_INVOKABLE void gitRestore(const QUrl &documentUrl, GitRestoreMode::Value mode);

    Q_INVOKABLE void gitIgnore(const QUrl &documentUrl, bool status);

    Q_INVOKABLE void gitProxyGet() const;

    Q_INVOKABLE void gitProxySet(const QString &localHttpProxy, const QString &localHttpsProxy, const QString &globalHttpProxy, const QString &globalHttpsProxy) const;

signals:
    void gitStatusChanged();

    void updateIndex();

    void openDocument(const QUrl &documentUrl);

    void addFinish();

    void appendBackground(int &taskId, const std::function<void()> &abort, const std::function<void()> &info);

    void removeBackground(int taskId);

    void refreshBackground(int taskId, const QString &message);

private:
    void processEnqueue(int command, const QStringList &arguments);

    void processDequeue();

    void processFinished(int exitcode);

    GitService *const m_gitService;
    GitOperationState::Value m_gitStatus{};
    QJsonObject m_config{};
    QQuickWidget *m_widget{};
    QObject *m_root{};
    QObject *m_continueDialog{};
    QObject *m_errorDialog{};
    QObject *m_remoteAddDialog{};
    QObject *m_canvas{};
    QObject *m_subjectLabel{};
    QObject *m_dateLabel{};
    QObject *m_authorLabel{};

    QQuickView *m_commitWindow{};
    QObject *m_commitRoot{};

    QQuickView *m_pushWindow{};
    QObject *m_pushRoot{};
    QObject *m_subjectLabel_{};
    QObject *m_dateLabel_{};
    QObject *m_authorLabel_{};

    int m_command{};
    QProcess *m_process{};
    QQueue<QVariantHash> m_queue{};
    QFileSystemWatcher *m_repoWatcher{};
    QTimer *m_repoWatcherTimer{};
    QFileSystemWatcher *m_indexWatcher{};
    QTimer *m_indexWatcherTimer{};
    QFileSystemWatcher *m_branchWatcher{};
    QTimer *m_branchWatcherTimer{};
    QString m_current{};
    bool m_remote{};
    QString m_upstream{};
    int m_taskId{-1};

    GitConfig *m_gitConfig{};

    BranchModel *m_branchModel{};
    LogModel *m_logModel{};
    ShowModel *m_showModel{};

    StatusModel *m_workingTreeModel{};
    StatusModel *m_indexModel{};

    CommitModel *m_commitModel{};
    ShowModel *m_showModel_{};

    struct GitCommand {
        enum {
            Null,
            Init,
            Status,
            Add,
            Commit,
            Fetch,
            Ahead,
            Diff_,
            ShowCommit_,
            Push,
            RemoteAdd,
            RemoteGet,
            UpstreamSet,
            UpstreamUnset,
            UpstreamGet,
            Branch,
            Switch,
            Create,
            Rename,
            Delete,
            Log,
            Reset,
            ShowCommit,
            ShowFile,
            Merge,
            Rebase,
            CherryPick,
            Abort,
            Continue,
            Diff,
            Restore,
            Ignore
        };
    };
};

class BranchModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit BranchModel(QObject *parent = nullptr);

    enum Role {
        TypeRole = Qt::UserRole + 1,
        HashRole,
        CommitRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

signals:
    void emptyChanged();
};

class LogModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        HashRole = Qt::UserRole + 1,
        PositionRole,
        ParentRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
};

class ShowModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(QString hash READ hashGet NOTIFY hashChanged)

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        DocumentUrlRole = Qt::UserRole + 1,
        StatusRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString hashGet() const {
        return m_hash;
    }

    void hashSet(const QString &hash) {
        if (m_hash == hash) return;
        m_hash = hash;
        emit hashChanged();
    }

signals:
    void hashChanged();

private:
    QString m_hash{};
};

class StatusModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit StatusModel(QObject *parent = nullptr);

    enum Role {
        DocumentUrlRole = Qt::UserRole + 1,
        StatusRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

signals:
    void emptyChanged();
};

class CommitModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit CommitModel(QObject *parent = nullptr);

    enum Role {
        HashRole = Qt::UserRole + 1
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

signals:
    void emptyChanged();
};

#endif //UNICOMM_GITMODULE_H

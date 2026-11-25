#ifndef UNICOMM_GITMODULE_H
#define UNICOMM_GITMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QFuture>
#include <QJsonObject>
#include <QQueue>
#include <QStandardItemModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/gitTypes.h"

class QProcess;
class QQuickView;
class QQuickWidget;

class GitConfig;
class GitService;
class ShowModel;
class StatusModel;
class CommitModel;

class GitModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("GitModule is created by C++.")
    Q_PROPERTY(GitOperationState::Value gitStatus READ gitStatusGet NOTIFY gitStatusChanged)
    Q_PROPERTY(QStandardItemModel* refs MEMBER m_refs CONSTANT)
    Q_PROPERTY(QString current READ currentGet NOTIFY currentChanged)
    Q_PROPERTY(QStandardItemModel* log MEMBER m_log CONSTANT)
    Q_PROPERTY(QStandardItemModel* show MEMBER m_show CONSTANT)
    Q_PROPERTY(int laneCount READ laneCountGet NOTIFY logChanged)

public:
    enum RefsRole { RefRole = Qt::UserRole + 1, TypeRole };
    Q_ENUM(RefsRole)

    enum LogRole { HashRole = Qt::UserRole + 1, PositionRole, ParentsRole };
    Q_ENUM(LogRole)

    explicit GitModule();

    ~GitModule() override;

    [[nodiscard]] GitOperationState::Value gitStatusGet() const {
        return m_gitStatus;
    }

    void gitStatusSet(const GitOperationState::Value state) {
        m_gitStatus = state;
        emit gitStatusChanged();
    }

    void propertySet();

    void saveState() const;

    void restoreState() const;

    // public: repo
    Q_INVOKABLE void init();

    void gitRepo();

    // public: branch
    [[nodiscard]] const QString &currentGet() const { return m_current; }

    void upstreamGet();

    Q_INVOKABLE void upstreamSet(const QString &upstream);

    Q_INVOKABLE void upstreamUnset();

    Q_INVOKABLE void refsGet();

    Q_INVOKABLE void branchCreate(const QString &startPoint, const QString &name, bool checkout);

    Q_INVOKABLE void branchSwitch(const QString &name);

    Q_INVOKABLE void branchRename(const QString &oldName, const QString &newName);

    Q_INVOKABLE void branchDelete(const QString &name);

    // public: history
    [[nodiscard]] int laneCountGet() const { return m_laneCount; }

    Q_INVOKABLE void logGet(const QString &ref);

    Q_INVOKABLE void showGet(const QString &hash);

    // public: working tree
    void statusGet();

    Q_INVOKABLE QFuture<bool> add(const QUrl &url = QUrl());

    Q_INVOKABLE void untrack(const QUrl &url);

    Q_INVOKABLE void ignore(const QUrl &url);

    Q_INVOKABLE void unignore(const QUrl &url);

    // public: legacy
    Q_INVOKABLE void gitCommitPre();

    Q_INVOKABLE void gitCommit(const QString &subject);

    Q_INVOKABLE void gitFetch();

    Q_INVOKABLE void gitPushPre();

    void gitAhead();

    Q_INVOKABLE void gitDiff_();

    Q_INVOKABLE void gitShowCommit_(const QString &hash);

    Q_INVOKABLE void gitPush();

    Q_INVOKABLE void gitRemoteAdd(const QString &upstreamUrl);

    void gitRemoteGet();

    Q_INVOKABLE void gitReset(const QString &hash, GitResetMode::Value mode);

    Q_INVOKABLE void gitCherryPick(const QString &hash);

    Q_INVOKABLE void gitShowFile(const QUrl &documentUrl);

    Q_INVOKABLE void gitMerge(const QString &name);

    Q_INVOKABLE void gitRebase(const QString &name);

    Q_INVOKABLE void gitAbort();

    Q_INVOKABLE void gitContinue(const QString &message);

    Q_INVOKABLE void gitDiff();

    Q_INVOKABLE void gitRestore(const QUrl &documentUrl, GitRestoreMode::Value mode);

    Q_INVOKABLE void gitProxyGet() const;

    Q_INVOKABLE void gitProxySet(const QString &localHttpProxy, const QString &localHttpsProxy, const QString &globalHttpProxy, const QString &globalHttpsProxy) const;

signals:
    void gitStatusChanged();

    void showContinueDialog();

    void showErrorDialog(const QString &title, const QString &text);

    void showProxyDialog(const QVariantHash &proxies);

    void showRemoteAddDialog();

    void currentChanged();

    void expandChanges();

    void resetCommit();

    void logChanged();

    void setCommitDetails(const QString &subject, const QString &date, const QString &author);

    void setPushCommitDetails(const QString &subject, const QString &date, const QString &author);

    void openDocument(const QUrl &documentUrl);

    int appendBackground(const std::function<void()> &abort, const std::function<void()> &info);

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
    QQuickView *m_commitWindow{};
    QQuickView *m_pushWindow{};

    int m_command{};
    QProcess *m_process{};
    QQueue<QVariantHash> m_queue{};
    QString m_current{};
    QString m_upstream{};
    int m_taskId{-1};

    GitConfig *m_gitConfig{};

    QStandardItemModel *m_refs{};
    QStandardItemModel *m_log{};
    int m_laneCount{};
    enum ShowRole { DocumentUrlRole = Qt::UserRole + 1, StatusRole };
    QStandardItemModel *m_show{};
    QString m_showHash{};

    StatusModel *m_workingTreeModel{};
    StatusModel *m_indexModel{};

    CommitModel *m_commitModel{};
    ShowModel *m_showModel_{};

    struct GitCommand {
        enum {
            Null,
            Commit,
            Fetch,
            Ahead,
            Diff_,
            ShowCommit_,
            Push,
            RemoteAdd,
            RemoteGet,
            Reset,
            ShowFile,
            Merge,
            Rebase,
            CherryPick,
            Abort,
            Continue,
            Diff,
            Restore
        };
    };
};

class ShowModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        DocumentUrlRole = Qt::UserRole + 1,
        StatusRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
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

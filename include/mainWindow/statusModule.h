#ifndef UNICOMM_STATUSMODULE_H
#define UNICOMM_STATUSMODULE_H

#include <QQuickWidget>
#include <QStandardItemModel>

class BackgroundModel;

class StatusModule final : public QQuickWidget {
    Q_OBJECT

public:
    explicit StatusModule(QWidget *parent = nullptr);

    ~StatusModule() override;

    void propertySet(const QVariantHash &objects);

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void backgroundAppend(int &taskId, const std::function<void()> &abort, const std::function<void()> &info);

    void backgroundRemove(int taskId);

    void backgroundRefresh(int taskId, const QString &message) const;

    Q_INVOKABLE void backgroundAbort(int taskId);

    Q_INVOKABLE void backgroundInfo(int taskId) const;

    Q_INVOKABLE void documentGoto(const QUrl &documentUrl);

    void documentFocus(const QUrl &documentUrl, const QVariantHash &session) const;

    void selectionChange(const QHash<QString, int> &selection) const;

    void threadRefresh(int run, int debug, int agent) const;

signals:
    void gotoDocument(const QUrl &documentUrl);

private:
    void backgroundUpdate() const;

    QObject *m_root{};
    QObject *m_positionButton{};
    QObject *m_eolModeButton{};
    QObject *m_codePageButton{};
    QObject *m_threadTooltip{};
    int m_taskId{};
    QHash<int, std::function<void()>> m_abortCallbacks{};
    QHash<int, std::function<void()>> m_infoCallbacks{};
    BackgroundModel *m_backgroundModel{};
};

class BackgroundModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(QString title READ titleGet NOTIFY titleChanged)
    Q_PROPERTY(int taskId READ taskIdGet NOTIFY taskIdChanged)
    Q_PROPERTY(int taskCount READ taskCountGet NOTIFY taskCountChanged)

public:
    explicit BackgroundModel(QObject *parent = nullptr);

    enum Role {
        TaskIdRole = Qt::UserRole + 1
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString titleGet() const {
        return m_title;
    }

    void titleSet(const QString &title) {
        if (m_title == title) return;
        m_title = title;
        emit titleChanged();
    }

    [[nodiscard]] int taskIdGet() const {
        return m_taskId;
    }

    void taskIdSet(const int taskId) {
        if (m_taskId == taskId) return;
        m_taskId = taskId;
        emit taskIdChanged();
    }

    [[nodiscard]] int taskCountGet() const {
        return rowCount();
    }

signals:
    void titleChanged();

    void taskIdChanged();

    void taskCountChanged();

private:
    QString m_title{};
    int m_taskId = 0;
};

#endif //UNICOMM_STATUSMODULE_H

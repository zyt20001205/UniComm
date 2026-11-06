#ifndef UNICOMM_THREADPOOL_H
#define UNICOMM_THREADPOOL_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QJsonObject>
#include <QStandardItemModel>

class QTableWidget;
class QQuickWidget;

class LuaInterpreter;
class ThreadpoolModel;

class ThreadpoolModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT

public:
    explicit ThreadpoolModule();

    ~ThreadpoolModule() override;

    void propertySet(const QVariantHash &objects);

    void quit();

    void threadStart(const QUrl &documentUrl, int mode, QString &threadId, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1);

    Q_INVOKABLE void threadStart(const QUrl &documentUrl, int mode, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1);

    Q_INVOKABLE void threadStop(const QString &threadId);

    Q_INVOKABLE [[nodiscard]] bool debugging() const;

    void inputWrite(const QString &threadId, const QByteArray &data) const;

    void stateSet(const QString &threadId, int state);

    Q_INVOKABLE void valueSet(const QString &threadId, const QString &documentUrl, const QString &expression, const QString &value, const QString &type);

signals:
    void openTerminal(const QString &id, int backend);

    void closeTerminal(const QString &id);

    void writeTerminal(const QString &id, const QByteArray &data);

    void finishThread(const QString &threadId, const QJsonObject &output);

    void trackQuit(float secondaryProgress, const QString &secondaryLog) const;

    void refreshThread(int run, int debug, int agent);

    void openDocument(const QUrl &documentUrl);

    void addMarker(const QUrl &documentUrl, int type, int line, int time);

    void deleteMarker(const QUrl &documentUrl, int type, int line);

    void insertCallStack(const QString &threadId, QStandardItemModel *callStackModel);

    void startDebug(const QString &threadId);

    void stopDebug(const QString &threadId);

    void appendLog(int type, const QString &prefix, const QString &message);

private:
    void threadAppend(int mode, const QString &name, const QString &threadId);

    void messageDialogNew(const QEventLoop *eventloop, const QString &threadId, const QString &text) const;

    QQuickWidget *m_widget{};
    QObject *m_mainItem{};
    QObject* m_errorDialog{};
    QHash<QString, QThread *> m_threadHash{};
    QHash<QString, LuaInterpreter *> m_interpreterHash{};
    QString m_agentTerminalId{};
    int m_run = 0;
    int m_debug = 0;
    int m_agent = 0;
    ThreadpoolModel *m_standardItemModel{};
    QString m_lifetime{};

    enum {
        ICON_COL,
        NAME_COL,
        SPAWN_COL,
        THREADID_COL
    };
};

class ThreadpoolModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit ThreadpoolModel(QObject *parent = nullptr);

    enum Role {
        ModeRole = Qt::UserRole + 1,
        StatusRole,
        ThreadIdRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

signals:
    void emptyChanged();
};

#endif //UNICOMM_THREADPOOL_H

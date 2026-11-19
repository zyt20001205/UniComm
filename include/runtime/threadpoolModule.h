#ifndef UNICOMM_THREADPOOL_H
#define UNICOMM_THREADPOOL_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QJsonObject>
#include <QVariantHash>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/runtimeTypes.h"

class QQuickWidget;

class LuaInterpreter;

class ThreadpoolModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ThreadpoolModule is created by C++.")
    Q_PROPERTY(QVariantHash threads READ threadsGet NOTIFY threadsChanged)

public:
    explicit ThreadpoolModule();

    ~ThreadpoolModule() override;

    void propertySet();

    [[nodiscard]] QVariantHash threadsGet() const { return m_threads; }

    void quit();

    Q_INVOKABLE QString threadStart(const QUrl &documentUrl, RunMode::Value mode, void *closure = nullptr);

    Q_INVOKABLE void threadStatusSet(const QString &threadId, ThreadStatus::Value status);

    void inputWrite(const QString &threadId, const QByteArray &data) const;

    void interpreterStateSet(const QString &threadId, InterpreterState::Value state) const;

    void watchExpressionsSet(const QMultiHash<QUrl, QString> &expressions) const;

    Q_INVOKABLE void watchValueRefresh(const QString &threadId) const;

    Q_INVOKABLE void watchValueSet(const QString &threadId, const QString &expression, const QString &value, const QString &type) const;

signals:
    void threadsChanged();

    void openTerminal(const QString &id, int backend);

    void closeTerminal(const QString &id);

    void writeTerminal(const QString &id, const QByteArray &data);

    void finishThread(const QString &threadId, const QJsonObject &output);

    void trackQuit(float secondaryProgress, const QString &secondaryLog) const;

    void setCallStack(const QString &threadId, const QVariantList &frames);

    void appendLog(int type, const QString &prefix, const QString &message);

private:
    [[nodiscard]] LuaInterpreter *interpreterGet(const QString &threadId) const;

    QQuickWidget *m_widget{};
    QVariantHash m_threads{};
};

#endif //UNICOMM_THREADPOOL_H

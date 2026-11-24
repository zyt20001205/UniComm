#ifndef UNICOMM_STATUSMODULE_H
#define UNICOMM_STATUSMODULE_H

#include <QQuickWidget>
#include <QtQmlIntegration/qqmlintegration.h>
#include <functional>

class StatusModule : public QQuickWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("StatusModule is created by C++.")
    Q_PROPERTY(QVariantList tasks READ tasksGet NOTIFY tasksChanged)

public:
    explicit StatusModule(QWidget *parent = nullptr);

    ~StatusModule() override;

    void propertySet();

    [[nodiscard]] const QVariantList &tasksGet() const { return m_tasks; }

    Q_INVOKABLE [[nodiscard]] QStringList documentPathGet(const QUrl &documentUrl) const;

    int backgroundAppend(const std::function<void()> &abort, const std::function<void()> &info);

    void backgroundRemove(int taskId);

    void backgroundRefresh(int taskId, const QString &message);

    Q_INVOKABLE void backgroundAbort(int taskId);

    Q_INVOKABLE void backgroundInfo(int taskId) const;

signals:
    void tasksChanged();

private:
    [[nodiscard]] qsizetype taskIndexGet(int taskId) const;

    int m_taskId{};
    QVariantList m_tasks{};
    QHash<int, std::function<void()>> m_abortCallbacks{};
    QHash<int, std::function<void()>> m_infoCallbacks{};
};

#endif //UNICOMM_STATUSMODULE_H

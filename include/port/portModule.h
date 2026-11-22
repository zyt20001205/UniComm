#ifndef UNICOMM_PORTMODULE_H
#define UNICOMM_PORTMODULE_H

#include <QJsonObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

#include "basePort.h"

class QQuickWidget;
class PortManageWindow;

class PortModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PortModule is created by C++.")
    Q_PROPERTY(QVariantList ports READ portsGet NOTIFY portsChanged)

public:
    explicit PortModule();

    ~PortModule() override;

    void propertySet();

    [[nodiscard]] const QVariantList &portsGet() const {
        return m_ports;
    }

    void portConfigSave();

    [[nodiscard]] QSet<QString> portList() const;

    [[nodiscard]] static QJsonObject portConfigGet(PortType::Value portType);

    [[nodiscard]] QString portCheck(const QJsonObject &portConfig, const QString &oldPortName = QString()) const;

    Q_INVOKABLE void portManage(int index = -1) const;

    [[nodiscard]] QString portInsert(int index, QJsonObject portConfig, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString portRemove(const QString &portName, const QString &undoGroupId = {});

    Q_INVOKABLE void portMove(int src, int dst);

    void portEdit(const QString &oldPortName, const QJsonObject &portConfig);

    Q_INVOKABLE void portToggle(int index);

    void portRefresh(const QString &portName, bool active);

    QHash<QString, BasePort *> m_portHash{};

signals:
    void portsChanged();

    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

private:
    static void portDefaults(QJsonObject &portConfig);

    [[nodiscard]] int portIndexGet(const QString &portName) const;

    void _portInsert(int index, const QJsonObject &portConfig);

    void _portRemove(int index);

    void _portEdit(int index, const QJsonObject &portConfig);

    void _portMove(int src, int dst);

    QQuickWidget *m_widget{};
    PortManageWindow *m_portManageWindow{};
    QVariantList m_ports{};
};

#endif //UNICOMM_PORTMODULE_H

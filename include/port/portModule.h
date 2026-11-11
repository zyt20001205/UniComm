#ifndef UNICOMM_PORTMODULE_H
#define UNICOMM_PORTMODULE_H

#include <QJsonObject>
#include <QStandardItemModel>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

#include "basePort.h"

class QPushButton;
class QQuickWidget;
class QTabWidget;
class QVBoxLayout;

class PortSetting;

class PortModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT

public:
    explicit PortModule();

    ~PortModule() override;

    void propertySet(const QVariantHash &objects);

    void portConfigSave();

    [[nodiscard]] QSet<QString> portList() const;

    [[nodiscard]] static QJsonObject portConfigGet(int portType);

    [[nodiscard]] QString portCheck(const QJsonObject &portConfig, const QString &oldPortName = QString()) const;

    Q_INVOKABLE void portSetting(int index = -1) const;

    [[nodiscard]] QString portInsert(int index, QJsonObject portConfig, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString portRemove(const QString &portName, const QString &undoGroupId = {});

    Q_INVOKABLE void portMove(int src, int dst);

    void portEdit(const QString &oldPortName, const QJsonObject &portConfig);

    Q_INVOKABLE void portToggle(int index);

    static void portRefresh(const QString &portName, bool active);

    QHash<QString, BasePort *> m_portHash{};
signals:
    void appendLog(int type, const QString &prefix, const QString &message);

private:
    static void portDefaults(QJsonObject &portConfig);

    void _portInsert(int index, const QJsonObject &portConfig);

    void _portRemove(const QString &portName);

    void _portEdit(int index, const QString &oldPortName, const QJsonObject &portConfig);

    void _portMove(int src, int dst);

    QQuickWidget *m_widget{};
    QObject *m_root{};
    PortSetting *m_portSetting{};
};

class PortModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit PortModel(QObject *parent = nullptr);

    enum Role {
        ActiveRole = Qt::UserRole + 1
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

    signals:
        void emptyChanged();
};

#endif //UNICOMM_PORTMODULE_H

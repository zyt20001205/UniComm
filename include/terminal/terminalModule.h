#ifndef UNICOMM_TERMINALMODULE_H
#define UNICOMM_TERMINALMODULE_H

#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/terminalTypes.h"

class QQuickView;
class QWidget;

class TerminalPage;

class TerminalModule : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("TerminalModule is created by C++.")
    Q_PROPERTY(QVariantList terminals READ terminalsGet NOTIFY terminalsChanged)

public:
    explicit TerminalModule(QWidget *parent = nullptr);

    ~TerminalModule() override;

    void propertySet(const QVariantHash &objects);

    void terminalConfigSave() const;

    Q_INVOKABLE void terminalManage() const;

    [[nodiscard]] QVariantList terminalsGet() const;

    Q_INVOKABLE void terminalSave(const QVariantList &terminals);

    [[nodiscard]] TerminalPage *terminalConstruct(const QUrl &terminalUrl, TerminalBackend::Value backend = TerminalBackend::Process);

    Q_INVOKABLE void terminalOpen(const QString &id, TerminalBackend::Value backend = TerminalBackend::Process);

    void terminalClose(const QString &id) const;

    void terminalWrite(const QString &id, const QByteArray &data) const;

signals:
    void terminalsChanged();

private:
    QJsonObject m_config{};
    QQuickView *m_manageWindow{};
    QObject *m_toolTip{};
    QObject *m_contextMenu{};

    QHash<QString, TerminalPage *> m_terminalPageHash{};
};

#endif //UNICOMM_TERMINALMODULE_H

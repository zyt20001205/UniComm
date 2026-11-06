#ifndef UNICOMM_TERMINALMODULE_H
#define UNICOMM_TERMINALMODULE_H

#include <QJsonObject>
#include <QStandardItemModel>
#include <QUrl>

#include "terminal/terminalPage.h"

class QQuickView;

class TerminalModel;

class TerminalModule final : public QObject {
    Q_OBJECT

public:
    explicit TerminalModule(QWidget *parent = nullptr);

    ~TerminalModule() override;

    void propertySet(const QVariantHash &objects);

    void terminalConfigSave() const;

    Q_INVOKABLE void terminalManage() const;

    Q_INVOKABLE void terminalSave();

    Q_INVOKABLE [[nodiscard]] int terminalAdd() const;

    Q_INVOKABLE void terminalDelete(int index) const;

    Q_INVOKABLE void terminalSwap(int src, int dst) const;

    [[nodiscard]] TerminalPage *terminalConstruct(const QUrl &terminalUrl, int backend = TerminalPage::Backend::Conpty);

    Q_INVOKABLE void terminalOpen(const QString &id, int backend = TerminalPage::Backend::Conpty);

    void terminalClose(const QString &id) const;

    void terminalWrite(const QString &id, const QByteArray &data) const;

private:
    QJsonObject m_config{};
    QQuickView *m_manageWindow{};
    QObject *m_toolTip{};
    QObject *m_contextMenu{};

    TerminalModel *m_terminalModel{};
    QHash<QString, TerminalPage *> m_terminalPageHash{};
};

class TerminalModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        IdRole = Qt::UserRole + 1,
        SessionRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int rowCountGet() const {
        return rowCount();
    }

signals:
    void rowCountChanged();
};

#endif //UNICOMM_TERMINALMODULE_H

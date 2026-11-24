#ifndef UNICOMM_HOOKMODULE_H
#define UNICOMM_HOOKMODULE_H

#include <QJsonArray>
#include <QStandardItemModel>

class HookModel;

class HookModule final : public QObject {
    Q_OBJECT

public:
    struct Event {
        enum {
            TurnStart,
            PermissionRequest,
            UserInputRequest,
            TurnFinish
        };
    };

    explicit HookModule(const QJsonArray &config, QObject *parent = nullptr);

    [[nodiscard]] HookModel *hookModelGet() const;

    [[nodiscard]] const QJsonArray &configGet() const;

    void hookEnabledSet(int event, bool enabled);

    void hookScriptInsert(int event, const QUrl &documentUrl);

    void hookScriptRemove(int event, const QUrl &documentUrl);

    void hookRun(int event) const;

private:
    [[nodiscard]] QVariantList scriptsGet(int event) const;

    void hookUpdate(int event) const;

    QJsonArray m_config{};
    HookModel *m_hookModel{};
};

class HookModel final : public QStandardItemModel {
    Q_OBJECT

public:
    explicit HookModel(QObject *parent = nullptr);

    enum Role {
        DescriptionRole = Qt::UserRole + 1,
        EnabledRole,
        ScriptsRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
};

#endif //UNICOMM_HOOKMODULE_H

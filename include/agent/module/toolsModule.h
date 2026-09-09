#ifndef UNICOMM_TOOLSMODULE_H
#define UNICOMM_TOOLSMODULE_H

#include <QFuture>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSet>

class SqlModule;
class McpModule;

struct ToolResult {
    QString content{};
    bool success{true};
};

class ToolsModule final : public QObject {
    Q_OBJECT

public:
    explicit ToolsModule(McpModule *mcpModule, SqlModule *sqlModule, QObject *parent = nullptr);

    ~ToolsModule() override = default;

    void initialize();

    void toolsRegister(const QJsonArray &tools);

    [[nodiscard]] QJsonArray toolsGet(const QSet<QString> &names, bool includeMcp) const;

    [[nodiscard]] QPair<bool, QString> toolCall(int mode, const QString &name, const QString &arguments) const;

    [[nodiscard]] QString toolTextGet(const QString &name, const QString &arguments) const;

    [[nodiscard]] QFuture<ToolResult> toolExecute(const QString &runtimeId, const QString &runtimeRole, const QString &name, const QString &arguments);

private:
    [[nodiscard]] static QJsonObject argumentsGet(const QString &arguments);

    [[nodiscard]] bool permissionGet(int mode, const QString &name) const;

    [[nodiscard]] ToolResult _toolExecute(const QString &runtimeId, const QString &name, const QJsonObject &object) const;

    QHash<QString, int> m_portTypes{};
    QJsonArray m_tools{};
    QJsonArray m_mcpTools{};
    McpModule *m_mcpModule{};
    SqlModule *m_sqlModule{};
    QSet<QString> m_writeGroup{};
    QSet<QString> m_fullAccessGroup{};
};

#endif //UNICOMM_TOOLSMODULE_H

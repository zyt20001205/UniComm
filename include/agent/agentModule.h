#ifndef UNICOMM_AGENTMODULE_H
#define UNICOMM_AGENTMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QHash>
#include <QStandardItemModel>
#include <QUrl>

#include "agent/runtime/runtimeModule.h"

class QQuickView;
class QQuickWidget;

class ConversationModel;
class ContextModule;
class EvalModule;
class HookModule;
class McpModule;
class ProviderModule;
class SqlModule;
class ToolsModule;
class ToastModule;

class AgentModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(int state READ stateGet NOTIFY changeState)

public:
    using AgentState = RuntimeModule::AgentState;
    using AgentMode = RuntimeModule::AgentMode;

    struct AgentStrategy {
        enum {
            Solo,
            Team
        };
    };

    explicit AgentModule();

    ~AgentModule() override;

    void propertySet(const QVariantHash &objects);

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void agentConfigSave();

    Q_INVOKABLE void agentManage() const;

    Q_INVOKABLE void evalOpen() const;

    [[nodiscard]] RuntimeServices runtimeServicesGet() const;

    [[nodiscard]] int stateGet() const;

    [[nodiscard]] QString undoGroupIdGet() const;

    Q_INVOKABLE void apikeySet(const QString &provider, const QString &apikey) const;

    Q_INVOKABLE void providerInsert(const QString &id, const QJsonObject &config);

    Q_INVOKABLE void providerEdit(const QString &id, const QJsonObject &config);

    Q_INVOKABLE void providerModelsGet(const QString &id) const;

    Q_INVOKABLE void providerRemove(const QString &id);

    Q_INVOKABLE [[nodiscard]] bool providerExists(const QString &id) const;

    Q_INVOKABLE void primaryModelSet(const QString &provider, const QString &model);

    Q_INVOKABLE void subagentModelSet(const QString &provider, const QString &model);

    Q_INVOKABLE void visionModelSet(const QString &provider, const QString &model);

    Q_INVOKABLE QString mcpInsert(const QUrl &url);

    Q_INVOKABLE void mcpRemove(const QUrl &url);

    Q_INVOKABLE void mcpEnabledSet(const QUrl &url, bool enabled);

    Q_INVOKABLE void hookEnabledSet(int event, bool enabled);

    Q_INVOKABLE void hookScriptInsert(int event, const QUrl &documentUrl);

    Q_INVOKABLE void hookScriptRemove(int event, const QUrl &documentUrl);

    Q_INVOKABLE [[nodiscard]] int compactThresholdGet() const;

    Q_INVOKABLE void compactThresholdSet(int threshold);

    Q_INVOKABLE [[nodiscard]] int toolFailureLimitGet() const;

    Q_INVOKABLE void toolFailureLimitSet(int limit);

    Q_INVOKABLE [[nodiscard]] int toolPlanIntervalGet() const;

    Q_INVOKABLE void toolPlanIntervalSet(int interval);

    Q_INVOKABLE [[nodiscard]] int toolCallLimitGet() const;

    Q_INVOKABLE void toolCallLimitSet(int limit);

    Q_INVOKABLE void conversationsGet();

    Q_INVOKABLE void conversationGet(const QString &id);

    Q_INVOKABLE void conversationInsert();

    Q_INVOKABLE void conversationRename(const QString &title);

    Q_INVOKABLE void conversationDelete();

    Q_INVOKABLE void attachmentAdd(const QUrl &documentUrl);

    Q_INVOKABLE void attachmentRemove(const QUrl &documentUrl);

    Q_INVOKABLE void conversationStrategySet(int strategy);

    Q_INVOKABLE void conversationModeSet(int mode);

    Q_INVOKABLE void conversationModelSet(const QString &provider, const QString &model);

    Q_INVOKABLE void conversationRollback();

    Q_INVOKABLE void changeRevert() const;

    Q_INVOKABLE void abort() const;

    Q_INVOKABLE void pre();

    Q_INVOKABLE void steer() const;

    Q_INVOKABLE void compact() const;

    Q_INVOKABLE void permission(const QString &runtimeId, bool status) const;

    Q_INVOKABLE void userInput(const QString &runtimeId, const QString &answer) const;

    void permissionRequest(const QString &runtimeId, const QString &message) const;

    void userInputRequest(const QString &runtimeId, const QVariantMap &request) const;

    void planUpdate(const QString &runtimeId, const QJsonObject &plan) const;

    void changeUpdate(const QString &undoGroupId) const;

    void diffUpdate(const QString &undoGroupId, const QVariantMap &fileDiffs, int additions, int deletions);

    [[nodiscard]] RuntimeModule *subagentDispatch(const QString &role, const QString &prompt, const QList<QUrl> &attachments = {});

    void subagentCreate(const QString &turnId, const QString &runtimeId, const QString &role, const QString &message) const;

    void subagentUpdate(const QString &runtimeId, const QString &message) const;

signals:
    void changeState();

private:
    void primaryRuntimeConnect(RuntimeModule *runtime);

    void modelUpdate(const QString &provider, const QString &model) const;

    void turnCreate(const QString &turnId, qint64 startedAt) const;

    void turnFinish(const QString &turnId, qint64 finishedAt) const;

    void chatCreate(const QString &turnId, const QString &messageId, const QString &role) const;

    void chatAppend(const QString &messageId, const QString &text) const;

    void chatReset(const QString &messageId) const;

    QJsonObject m_config{};
    QQuickWidget *m_widget{};
    QQuickView *m_evalWindow{};
    QQuickView *m_manageWindow{};
    QObject *m_root{};
    ToastModule *m_toast{};
    QObject *m_modeMenu{};
    QObject *m_conversationComboBox{};
    QObject *m_textArea{};
    QObject *m_strategyButton{};
    QObject *m_modeButton{};
    QObject *m_modelButton{};

    QString m_conversationId{};
    ConversationModel *m_conversationModel{};
    ContextModule *m_contextModule{};
    McpModule *m_mcpModule{};
    ProviderModule *m_providerModule{};
    SqlModule *m_sqlModule{};
    EvalModule *m_evalModule{};
    HookModule *m_hookModule{};
    ToolsModule *m_toolsModule{};
    QString m_general{};
    QString m_primary{};
    QString m_supervisor{};
    QList<QUrl> m_attachments{};
    QString m_undoGroupId{};
    QVariantMap m_fileDiffs{};
    int m_additions{};
    int m_deletions{};
    QHash<QString, RuntimeModule *> m_runtimes{};
};

class ConversationModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        IdRole = Qt::UserRole + 1
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
};

#endif //UNICOMM_AGENTMODULE_H

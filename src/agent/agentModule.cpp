#include "agent/agentModule.h"

#include <QDateTime>
#include <QFileInfo>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QUuid>

#include "globals.h"
#include "agent/module/contextModule.h"
#include "agent/module/evalModule.h"
#include "agent/module/hookModule.h"
#include "agent/module/mcpModule.h"
#include "agent/module/sqlModule.h"
#include "agent/module/toolsModule.h"
#include "agent/provider/baseProvider.h"
#include "agent/provider/providerModule.h"
#include "agent/role/dataAgent.h"
#include "agent/role/generalAgent.h"
#include "agent/role/hardwareAgent.h"
#include "agent/role/softwareAgent.h"
#include "agent/role/supervisorAgent.h"
#include "agent/role/visionAgent.h"
#include "core/globalManager.h"
#include "core/undoModule.h"
#include "document/documentModule.h"
#include "mainWindow/toastModule.h"
#include "util/uniCast.h"

// public
AgentModule::AgentModule()
    : DockWidget("Agent"),
      m_config(g_workspaceConfig["agentConfig"].toObject()),
      m_widget(new QQuickWidget()),
      m_evalWindow(new QQuickView()),
      m_manageWindow(new QQuickView()),
      m_conversationId(m_config["id"].toString()),
      m_conversationModel(new ConversationModel(this)),
      m_contextModule(new ContextModule(m_config["context"].toObject(), this)),
      m_mcpModule(new McpModule(m_config["mcp"].toObject(), this)),
      m_providerModule(new ProviderModule(m_config["providers"].toObject(), m_config["default"].toObject(), this)),
      m_sqlModule(new SqlModule(m_config["sql"].toObject(), this)),
      m_evalModule(new EvalModule(m_sqlModule, this)),
      m_hookModule(new HookModule(m_config["hooks"].toArray(), this)),
      m_toolsModule(new ToolsModule(m_mcpModule, m_sqlModule, this)) {
    connect(m_mcpModule, &McpModule::registerTools, m_toolsModule, &ToolsModule::toolsRegister);
    auto *general = new RuntimeModule(new GeneralAgent(), runtimeServicesGet(), this); // NOLINT
    m_general = general->idGet();
    m_runtimes.insert(m_general, general);
    auto *supervisor = new RuntimeModule(new SupervisorAgent(), runtimeServicesGet(), this); // NOLINT
    m_supervisor = supervisor->idGet();
    m_runtimes.insert(m_supervisor, supervisor);
    m_primary = m_general;
    setWidget(m_widget);

    conversationsGet();
}

AgentModule::~AgentModule() {
    delete m_evalWindow;
    delete m_manageWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void AgentModule::propertySet(const QVariantHash &objects) {
    m_toast = qvariant_cast<ToastModule *>(objects["mainWindowToast"]);
    m_hookModule->propertySet(objects);
    m_modeMenu = qvariant_cast<QObject *>(objects["agentModuleModeMenu"]);

    m_evalWindow->setTitle(tr("Agent Evaluation"));
    m_evalWindow->setTransientParent(g_mainWindow->windowHandle());
    m_evalWindow->rootContext()->setContextProperty("evalModel", m_evalModule->modelGet());
    m_evalWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_evalWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_evalWindow->setSource(QUrl("qrc:/qml/agent/agentEvalWindow.qml"));

    m_manageWindow->setTitle(tr("Agent Settings"));
    m_manageWindow->setTransientParent(g_mainWindow->windowHandle());
    m_manageWindow->rootContext()->setContextProperty("agentModule", this);
    m_manageWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_manageWindow->rootContext()->setContextProperty("hookModel", m_hookModule->hookModelGet());
    m_manageWindow->rootContext()->setContextProperty("mcpModel", m_mcpModule->mcpModelGet());
    m_manageWindow->rootContext()->setContextProperty("providerModel", m_providerModule->providerModelGet());
    m_manageWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_manageWindow->setSource(QUrl("qrc:/qml/agent/agentManageWindow.qml"));

    m_widget->rootContext()->setContextProperty("agentModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainLinkMenu", objects["mainWindowLinkMenu"]);
    m_widget->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_widget->rootContext()->setContextProperty("documentModule", objects["documentModule"]);
    m_widget->rootContext()->setContextProperty("fileModule", objects["fileModule"]);
    m_widget->rootContext()->setContextProperty("renameDialog", objects["agentModuleRenameDialog"]);
    m_widget->rootContext()->setContextProperty("conversationModel", m_conversationModel);
    m_widget->rootContext()->setContextProperty("modeMenu", m_modeMenu);
    m_widget->rootContext()->setContextProperty("modelMenu", objects["agentModuleModelMenu"]);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/agent/agentModule.qml"));
    m_root = m_widget->rootObject();

    primaryRuntimeConnect(m_runtimes.value(m_general));
    primaryRuntimeConnect(m_runtimes.value(m_supervisor));
    m_toolsModule->initialize();
    m_mcpModule->initialize();

    m_providerModule->propertySet(QVariantHash{
        {"agentModuleModelMenu", objects["agentModuleModelMenu"]}
    });
    connect(m_providerModule, &ProviderModule::modelsChanged, this, [this] {
        if (m_conversationId.isEmpty()) return;
        const auto conversation = m_sqlModule->conversationGet(m_conversationId).first;
        modelUpdate(conversation.provider, conversation.model);
    });
    m_providerModule->initialize();

    if (!m_conversationId.isEmpty()) {
        m_conversationComboBox->setProperty("currentValue", m_conversationId);
        conversationGet(m_conversationComboBox->property("currentValue").toString());
    }
}

void AgentModule::propertyGet(const QVariantMap &objects) {
    m_conversationComboBox = qvariant_cast<QObject *>(objects["conversationComboBox"]);
    m_textArea = qvariant_cast<QObject *>(objects["textArea"]);
    m_strategyButton = qvariant_cast<QObject *>(objects["strategyButton"]);
    m_modeButton = qvariant_cast<QObject *>(objects["modeButton"]);
    m_modelButton = qvariant_cast<QObject *>(objects["modelButton"]);
}

void AgentModule::agentConfigSave() {
    m_config["id"] = m_conversationId;
    g_workspaceConfig["agentConfig"] = m_config;
}

void AgentModule::agentManage() const {
    m_manageWindow->resize(1080, 720);
    m_manageWindow->show();
}

void AgentModule::evalOpen() const {
    m_evalWindow->resize(1080, 720);
    m_evalWindow->show();
}

RuntimeServices AgentModule::runtimeServicesGet() const {
    return {m_contextModule, m_providerModule, m_sqlModule, m_toolsModule};
}

int AgentModule::stateGet() const {
    return m_runtimes.value(m_primary)->stateGet();
}

QString AgentModule::undoGroupIdGet() const {
    return m_undoGroupId;
}

void AgentModule::apikeySet(const QString &provider, const QString &apikey) const {
    m_providerModule->apikeySet(provider, apikey);
}

void AgentModule::providerInsert(const QString &id, const QJsonObject &config) {
    const auto providerId = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    auto providers = m_config["providers"].toObject();
    providers[providerId] = config;
    m_config["providers"] = providers;
    m_providerModule->providerInsert(providerId, config);
    agentConfigSave();
}

void AgentModule::providerEdit(const QString &id, const QJsonObject &config) {
    auto providers = m_config["providers"].toObject();
    providers[id] = config;
    m_config["providers"] = providers;
    m_providerModule->providerEdit(id, config);
    agentConfigSave();
}

void AgentModule::providerModelsGet(const QString &id) const {
    m_providerModule->providerModelsGet(id);
}

void AgentModule::providerRemove(const QString &id) {
    auto providers = m_config["providers"].toObject();
    providers.remove(id);
    m_config["providers"] = providers;
    m_providerModule->providerRemove(id);
    agentConfigSave();
}

bool AgentModule::providerExists(const QString &id) const {
    return m_providerModule->providerExists(id);
}

void AgentModule::primaryModelSet(const QString &provider, const QString &model) {
    auto defaults = m_config["default"].toObject();
    defaults["primary"] = QJsonObject{{"provider", provider}, {"model", model}};
    m_config["default"] = defaults;
    m_providerModule->defaultSet(defaults);
    agentConfigSave();
}

void AgentModule::subagentModelSet(const QString &provider, const QString &model) {
    auto defaults = m_config["default"].toObject();
    defaults["subagent"] = QJsonObject{{"provider", provider}, {"model", model}};
    m_config["default"] = defaults;
    m_providerModule->defaultSet(defaults);
    agentConfigSave();
}

void AgentModule::visionModelSet(const QString &provider, const QString &model) {
    auto defaults = m_config["default"].toObject();
    defaults["vision"] = QJsonObject{{"provider", provider}, {"model", model}};
    m_config["default"] = defaults;
    m_providerModule->defaultSet(defaults);
    agentConfigSave();
}

QString AgentModule::mcpInsert(const QUrl &url) {
    const auto error = m_mcpModule->serverInsert(url);
    if (!error.isEmpty()) return error;

    auto mcp = m_config["mcp"].toObject();
    mcp[url.toString()] = true;
    m_config["mcp"] = mcp;
    agentConfigSave();
    return {};
}

void AgentModule::mcpRemove(const QUrl &url) {
    auto mcp = m_config["mcp"].toObject();
    mcp.remove(url.toString());
    m_config["mcp"] = mcp;
    m_mcpModule->serverRemove(url);
    agentConfigSave();
}

void AgentModule::mcpEnabledSet(const QUrl &url, const bool enabled) {
    auto mcp = m_config["mcp"].toObject();
    mcp[url.toString()] = enabled;
    m_config["mcp"] = mcp;
    m_mcpModule->enabledSet(url, enabled);
    agentConfigSave();
}

void AgentModule::hookEnabledSet(const int event, const bool enabled) {
    m_hookModule->hookEnabledSet(event, enabled);
    m_config["hooks"] = m_hookModule->configGet();
    agentConfigSave();
}

void AgentModule::hookScriptInsert(const int event, const QUrl &documentUrl) {
    m_hookModule->hookScriptInsert(event, documentUrl);
    m_config["hooks"] = m_hookModule->configGet();
    agentConfigSave();
}

void AgentModule::hookScriptRemove(const int event, const QUrl &documentUrl) {
    m_hookModule->hookScriptRemove(event, documentUrl);
    m_config["hooks"] = m_hookModule->configGet();
    agentConfigSave();
}

int AgentModule::compactThresholdGet() const {
    return m_contextModule->compactThresholdGet();
}

void AgentModule::compactThresholdSet(const int threshold) {
    m_contextModule->compactThresholdSet(threshold);
    auto context = m_config["context"].toObject();
    context["compactThreshold"] = threshold;
    m_config["context"] = context;
    agentConfigSave();
}

int AgentModule::toolFailureLimitGet() const {
    return m_config["runtime"].toObject()["toolFailureLimit"].toInt();
}

void AgentModule::toolFailureLimitSet(const int limit) {
    auto runtime = m_config["runtime"].toObject();
    runtime["toolFailureLimit"] = limit;
    m_config["runtime"] = runtime;
    agentConfigSave();
}

int AgentModule::toolPlanIntervalGet() const {
    return m_config["runtime"].toObject()["toolPlanInterval"].toInt();
}

void AgentModule::toolPlanIntervalSet(const int interval) {
    auto runtime = m_config["runtime"].toObject();
    runtime["toolPlanInterval"] = interval;
    m_config["runtime"] = runtime;
    agentConfigSave();
}

int AgentModule::toolCallLimitGet() const {
    return m_config["runtime"].toObject()["toolCallLimit"].toInt();
}

void AgentModule::toolCallLimitSet(const int limit) {
    auto runtime = m_config["runtime"].toObject();
    runtime["toolCallLimit"] = limit;
    m_config["runtime"] = runtime;
    agentConfigSave();
}

// public: conversation management
void AgentModule::conversationsGet() {
    const auto conversationId = m_conversationId;
    const auto conversations = m_sqlModule->conversationsGet();
    auto currentIndex = -1;
    SqlModule::Conversation currentConversation{};

    m_conversationModel->clear();
    for (const auto &conversation: conversations) {
        auto *item = new QStandardItem(conversation.title); // NOLINT
        item->setData(conversation.id, ConversationModel::IdRole);
        m_conversationModel->appendRow(item);

        if (!currentConversation.id.isEmpty() && conversation.id != conversationId) continue;
        currentIndex = m_conversationModel->rowCount() - 1;
        currentConversation = conversation;
    }

    m_conversationId = currentConversation.id;
    m_evalModule->update(m_conversationId);
    const auto strategy = currentConversation.id.isEmpty() ? AgentStrategy::Solo : currentConversation.strategy;
    m_primary = strategy == AgentStrategy::Solo ? m_general : m_supervisor;

    if (m_conversationComboBox == nullptr || m_strategyButton == nullptr || m_modeButton == nullptr || m_modelButton == nullptr) return;
    m_conversationComboBox->setProperty("currentIndex", currentIndex);
    m_strategyButton->setProperty("strategy", strategy);
    m_modeButton->setProperty("mode", currentConversation.id.isEmpty() ? AgentMode::Chat : currentConversation.mode);
    m_modeMenu->setProperty("selectedIndex", currentConversation.id.isEmpty() ? AgentMode::Chat : currentConversation.mode);
    modelUpdate(currentConversation.provider, currentConversation.model);
}

void AgentModule::conversationGet(const QString &id) {
    if (m_strategyButton == nullptr || m_modeButton == nullptr || m_modelButton == nullptr) return;
    QMetaObject::invokeMethod(m_root, "chatClear");
    const auto [conversation, messages] = m_sqlModule->conversationGet(id);
    if (conversation.id.isEmpty()) {
        m_conversationId.clear();
        m_evalModule->update({});
        m_primary = m_general;
        m_strategyButton->setProperty("strategy", AgentStrategy::Solo);
        m_modeButton->setProperty("mode", AgentMode::Chat);
        m_modeMenu->setProperty("selectedIndex", AgentMode::Chat);
        modelUpdate({}, {});
        return;
    }

    m_conversationId = id;
    m_evalModule->update(m_conversationId);
    m_primary = conversation.strategy == AgentStrategy::Solo ? m_general : m_supervisor;
    QString turnId{};
    qint64 finishedAt{};
    QHash<QString, QPair<QString, QString> > toolCalls{};
    for (const auto &message: messages) {
        const auto &role = message.role;
        if (message.turnId != turnId) {
            if (!turnId.isEmpty()) turnFinish(turnId, finishedAt);
            turnId = message.turnId;
            turnCreate(turnId, message.timing.createdAt);
        }
        finishedAt = message.timing.finishedAt;
        for (const auto &value: message.toolCalls) {
            const auto object = value.toObject();
            const auto function = object.value("function").toObject();
            const auto id = object.value("id").toString();
            toolCalls[id] = {function.value("name").toString(), function.value("arguments").toString()};
        }
        if (role == "tool") {
            const auto toolCall = toolCalls.value(message.toolCallId);
            chatCreate(turnId, message.id, role, message.attachments);
            chatAppend(message.id, m_toolsModule->toolTextGet(toolCall.first, toolCall.second));
            chatAppend(message.id, message.approved ? " ✓" : " ✗");
            continue;
        }
        const auto &content = message.content;
        if (!content.isEmpty() || !message.attachments.isEmpty()) {
            chatCreate(turnId, message.id, role, message.attachments);
            chatAppend(message.id, content);
        }
    }
    if (!turnId.isEmpty()) turnFinish(turnId, finishedAt);
    m_strategyButton->setProperty("strategy", conversation.strategy);
    m_modeButton->setProperty("mode", conversation.mode);
    m_modeMenu->setProperty("selectedIndex", conversation.mode);
    modelUpdate(conversation.provider, conversation.model);
    QMetaObject::invokeMethod(m_root, "usageUpdate", Q_ARG(double, conversation.contextTokens));
    QMetaObject::invokeMethod(m_root, "followToTail", Qt::QueuedConnection);
}

void AgentModule::conversationInsert() {
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto timestamp = QDateTime::currentMSecsSinceEpoch();
    const auto primary = m_config["default"].toObject().value("primary").toObject();
    m_sqlModule->conversationInsert(SqlModule::Conversation{
        .id = id,
        .title = id,
        .strategy = AgentStrategy::Solo,
        .provider = primary.value("provider").toString(),
        .model = primary.value("model").toString(),
        .createdAt = timestamp,
        .updatedAt = timestamp
    });
    m_conversationId = id;
    conversationsGet();
}

void AgentModule::conversationRename(const QString &title) {
    if (title.isEmpty()) return;
    m_sqlModule->conversationRename(m_conversationId, title);
    conversationsGet();
}

void AgentModule::conversationDelete() {
    m_sqlModule->conversationDelete(m_conversationId);
    conversationsGet();
}

void AgentModule::attachmentAdd(const QUrl &documentUrl) {
    if (m_attachments.contains(documentUrl)) return;
    m_attachments.append(documentUrl);
    QMetaObject::invokeMethod(
        m_root,
        "attachmentAdd",
        Q_ARG(QUrl, documentUrl),
        Q_ARG(QString, QFileInfo(documentUrl.toLocalFile()).fileName()),
        Q_ARG(QUrl, uni_cast<QFileIcon>(documentUrl).value)
    );
}

void AgentModule::attachmentRemove(const QUrl &documentUrl) {
    const auto index = m_attachments.indexOf(documentUrl);
    m_attachments.removeAt(index);
    QMetaObject::invokeMethod(m_root, "attachmentRemove", Q_ARG(int, index));
}

void AgentModule::conversationStrategySet(const int strategy) {
    if (m_conversationId.isEmpty()) conversationInsert();
    m_sqlModule->conversationStrategySet(m_conversationId, strategy);
    conversationsGet();
}

void AgentModule::conversationModeSet(const int mode) {
    if (m_conversationId.isEmpty()) conversationInsert();
    m_sqlModule->conversationModeSet(m_conversationId, mode);
    conversationsGet();
}

void AgentModule::conversationModelSet(const QString &provider, const QString &model) {
    if (m_conversationId.isEmpty()) conversationInsert();
    m_sqlModule->conversationModelSet(m_conversationId, provider, model);
    modelUpdate(provider, model);
}

void AgentModule::conversationRollback() {
    const auto messages = m_sqlModule->conversationGet(m_conversationId).second;
    if (messages.isEmpty()) return;
    for (auto i = messages.size() - 1; i >= 0; --i) {
        const auto &message = messages.at(i);
        if (message.role == "user") {
            m_textArea->setProperty("text", message.content);
            m_sqlModule->conversationRollback(m_conversationId, message.turnId);
            break;
        }
    }
    conversationGet(m_conversationId);
}

void AgentModule::changeRevert() const {
    const auto error = g_undo->undoGroupRevert(m_undoGroupId);
    if (!error.isEmpty()) m_toast->show(ToastLevel::Error, tr("Agent"), error);
}

// public: state transition
void AgentModule::abort() const {
    auto *primary = m_runtimes.value(m_primary);
    primary->abort();
    QMetaObject::invokeMethod(m_root, "requestsClear");
    const auto runtimes = m_runtimes.values();
    for (auto *runtime: runtimes) {
        if (runtime == primary || runtime->roleGet() == "general" || runtime->roleGet() == "supervisor") continue;
        runtime->abort();
    }
}

void AgentModule::pre() {
    if (m_conversationComboBox->property("currentValue").toString().isEmpty()) conversationInsert();
    m_runtimes.value(m_primary)->pre(m_conversationId, m_textArea->property("text").toString(), m_attachments);
}

void AgentModule::steer() const {
    m_runtimes.value(m_primary)->steer(m_textArea->property("text").toString());
    QMetaObject::invokeMethod(m_textArea, "clear");
}

void AgentModule::compact() const {
    m_runtimes.value(m_primary)->compact(m_conversationId);
}

void AgentModule::permission(const QString &runtimeId, const bool status) const {
    if (auto *runtime = m_runtimes.value(runtimeId)) runtime->permission(status);
}

void AgentModule::userInput(const QString &runtimeId, const QString &answer) const {
    if (auto *runtime = m_runtimes.value(runtimeId)) runtime->userInput(answer);
}

// public: frontend
void AgentModule::permissionRequest(const QString &runtimeId, const QString &message) const {
    const auto *runtime = m_runtimes.value(runtimeId);
    if (runtime == nullptr) return;
    QMetaObject::invokeMethod(m_root, "permissionRequest",Q_ARG(QString, runtimeId),Q_ARG(QString, runtime->roleGet()),Q_ARG(QString, message));
    m_hookModule->hookRun(HookModule::Event::PermissionRequest);
}

void AgentModule::userInputRequest(const QString &runtimeId, const QVariantMap &request) const {
    const auto *runtime = m_runtimes.value(runtimeId);
    if (runtime == nullptr) return;
    QMetaObject::invokeMethod(m_root, "userInputRequest",Q_ARG(QString, runtimeId),Q_ARG(QString, runtime->roleGet()),Q_ARG(QVariant, request));
    m_hookModule->hookRun(HookModule::Event::UserInputRequest);
}

void AgentModule::planUpdate(const QString &runtimeId, const QJsonObject &plan) const {
    QMetaObject::invokeMethod(m_root, "planUpdate", Q_ARG(QVariant, plan.toVariantMap()));
}

RuntimeModule *AgentModule::subagentDispatch(const QString &role, const QString &prompt, const QList<QUrl> &attachments) {
    const auto conversation = m_sqlModule->conversationGet(m_conversationId).first;
    const auto _default = m_config["default"].toObject().value(role == "vision" ? "vision" : "subagent").toObject();
    const auto provider = _default.value("provider").toString(conversation.provider);
    const auto model = _default.value("model").toString(conversation.model);

    BaseAgent *agent{};
    if (role == "data") agent = new DataAgent();
    if (role == "hardware") agent = new HardwareAgent();
    if (role == "software") agent = new SoftwareAgent();
    if (role == "vision") agent = new VisionAgent();
    if (agent == nullptr) return nullptr;

    auto *worker = new RuntimeModule(agent, runtimeServicesGet(), this); // NOLINT
    m_runtimes.insert(worker->idGet(), worker);
    subagentCreate(m_runtimes.value(m_primary)->turnIdGet(), worker->idGet(), role, prompt);
    connect(worker, &RuntimeModule::finishRun, worker, [this, worker](const QString &result, const bool) {
        subagentUpdate(worker->idGet(), result);
        m_runtimes.remove(worker->idGet());
        worker->deleteLater();
    });
    connect(worker, &RuntimeModule::retryRequest, worker, [this, worker](const int attempt, const int limit) {
        subagentUpdate(worker->idGet(), tr("Connection lost. Retrying %1/%2...").arg(attempt).arg(limit));
    });
    worker->request(provider, model, conversation.mode, prompt, attachments);
    return worker;
}

void AgentModule::subagentCreate(const QString &turnId, const QString &runtimeId, const QString &role, const QString &message) const {
    QMetaObject::invokeMethod(m_root, "subagentCreate", Q_ARG(QString, turnId), Q_ARG(QString, runtimeId), Q_ARG(QString, role), Q_ARG(QString, message));
}

void AgentModule::subagentUpdate(const QString &runtimeId, const QString &message) const {
    QMetaObject::invokeMethod(m_root, "subagentUpdate", Q_ARG(QString, runtimeId), Q_ARG(QString, message));
}

// private
void AgentModule::primaryRuntimeConnect(RuntimeModule *runtime) {
    connect(runtime, &RuntimeModule::changeState, this, [this, runtime] {
        if (runtime != m_runtimes.value(m_primary)) return;
        emit changeState();
        const auto turnId = runtime->turnIdGet();
        switch (runtime->stateGet()) {
            case AgentState::Ready:
                QMetaObject::invokeMethod(m_root, "activityFinish", Q_ARG(QString, turnId));
                break;
            case AgentState::Compact:
                QMetaObject::invokeMethod(m_root, "compactStart", Q_ARG(QString, turnId));
                break;
            case AgentState::Request:
            case AgentState::Think:
                QMetaObject::invokeMethod(m_root, "thinkingStart", Q_ARG(QString, turnId));
                break;
            case AgentState::Abort:
            case AgentState::Error:
            case AgentState::Complete:
            case AgentState::Response:
            case AgentState::ToolCall:
            case AgentState::Permission:
            case AgentState::UserInput:
            case AgentState::ToolExec:
                QMetaObject::invokeMethod(m_root, "activityFinish", Q_ARG(QString, turnId));
                break;
            default: break;
        }
    });
    connect(runtime, &RuntimeModule::showError, this, [this, runtime](const QString &message) {
        if (runtime != m_runtimes.value(m_primary)) return;
        m_toast->show(ToastLevel::Error, tr("Agent"), message);
    });
    connect(runtime, &RuntimeModule::createTurn, this, [this, runtime](const QString &turnId, const qint64 startedAt) {
        if (runtime != m_runtimes.value(m_primary)) return;
        m_fileDiffs.clear();
        m_additions = 0;
        m_deletions = 0;
        if (!m_undoGroupId.isEmpty()) g_undo->undoGroupRelease(m_undoGroupId);
        m_undoGroupId = g_undo->undoGroupBegin();
        g_document->transactionBegin(m_undoGroupId);
        turnCreate(turnId, startedAt);
        m_attachments.clear();
        QMetaObject::invokeMethod(m_root, "attachmentsClear");
        QMetaObject::invokeMethod(m_textArea, "clear");
        m_hookModule->hookRun(HookModule::Event::TurnStart);
    });
    connect(runtime, &RuntimeModule::finishTurn, this, [this, runtime](const QString &turnId, const qint64 finishedAt) {
        if (runtime != m_runtimes.value(m_primary)) return;
        const auto documentError = g_document->transactionCommit(m_undoGroupId);
        if (!documentError.isEmpty()) m_toast->show(ToastLevel::Error, tr("Agent"), documentError);
        const auto commitError = g_undo->undoGroupCommit(
            m_undoGroupId,
            tr("Agent Change"),
            [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Agent"), error); });
        if (!commitError.isEmpty()) m_toast->show(ToastLevel::Error, tr("Agent"), commitError);
        turnFinish(turnId, finishedAt);
        m_evalModule->update(m_conversationId);
        m_hookModule->hookRun(HookModule::Event::TurnFinish);
    });
    connect(runtime, &RuntimeModule::createChat, this, [this, runtime](const QString &turnId, const QString &messageId, const QString &role, const QList<QUrl> &attachments) {
        if (runtime == m_runtimes.value(m_primary)) chatCreate(turnId, messageId, role, attachments);
    });
    connect(runtime, &RuntimeModule::appendChat, this, [this, runtime](const QString &messageId, const QString &text) {
        if (runtime == m_runtimes.value(m_primary)) chatAppend(messageId, text);
    });
    connect(runtime, &RuntimeModule::resetChat, this, [this, runtime](const QString &messageId) {
        if (runtime == m_runtimes.value(m_primary)) chatReset(messageId);
    });
    connect(runtime, &RuntimeModule::retryRequest, this, [this, runtime](const int attempt, const int limit) {
        if (runtime != m_runtimes.value(m_primary)) return;
        QMetaObject::invokeMethod(m_root, "reconnectStart", Q_ARG(QString, runtime->turnIdGet()), Q_ARG(int, attempt), Q_ARG(int, limit));
    });
    connect(runtime, &RuntimeModule::updateUsage, this, [this, runtime](const qint64 totalTokens) {
        if (runtime != m_runtimes.value(m_primary)) return;
        QMetaObject::invokeMethod(m_root, "usageUpdate", Q_ARG(double, totalTokens));
    });
}

void AgentModule::diffUpdate(const QString &undoGroupId, const QVariantMap &fileDiffs, const int additions, const int deletions) {
    if (undoGroupId != m_undoGroupId) return;
    m_fileDiffs = fileDiffs;
    m_additions = additions;
    m_deletions = deletions;
    changeUpdate(undoGroupId);
}

void AgentModule::changeUpdate(const QString &undoGroupId) const {
    if (undoGroupId != m_undoGroupId) return;
    auto changes = g_undo->undoGroupGet(m_undoGroupId);
    changes.insert("fileDiffs", m_fileDiffs);
    changes.insert("additions", m_additions);
    changes.insert("deletions", m_deletions);
    QMetaObject::invokeMethod(m_root, "changeUpdate", Q_ARG(QVariant, changes));
}

void AgentModule::modelUpdate(const QString &provider, const QString &model) const {
    const auto providerInstance = m_providerModule->providerGet(provider);
    if (providerInstance == nullptr || model.isEmpty()) {
        m_modelButton->setProperty("text", tr("Select model"));
        QMetaObject::invokeMethod(m_root, "modelUpdate", Q_ARG(double, 0));
        return;
    }
    const auto modelInfo = providerInstance->modelGet(model);
    m_modelButton->setProperty("text", modelInfo.name);
    QMetaObject::invokeMethod(m_root, "modelUpdate", Q_ARG(double, modelInfo.contextWindow));
}

void AgentModule::turnCreate(const QString &turnId, const qint64 startedAt) const {
    QMetaObject::invokeMethod(m_root, "turnCreate", Q_ARG(QString, turnId), Q_ARG(double, startedAt));
}

void AgentModule::turnFinish(const QString &turnId, const qint64 finishedAt) const {
    QMetaObject::invokeMethod(m_root, "turnFinish", Q_ARG(QString, turnId), Q_ARG(double, finishedAt));
}

void AgentModule::chatCreate(const QString &turnId, const QString &messageId, const QString &role, const QList<QUrl> &attachments) const {
    QVariantList items{};
    for (const auto &attachment: attachments) {
        items.append(QVariantMap{
            {"attachmentUrl", attachment},
            {"fileName", QFileInfo(attachment.toLocalFile()).fileName()},
            {"iconSource", uni_cast<QFileIcon>(attachment).value}
        });
    }
    QMetaObject::invokeMethod(
        m_root,
        "chatCreate",
        Q_ARG(QString, turnId),
        Q_ARG(QString, messageId),
        Q_ARG(QString, role),
        Q_ARG(QVariant, items)
    );
}

void AgentModule::chatAppend(const QString &messageId, const QString &text) const {
    QMetaObject::invokeMethod(m_root, "chatAppend", Q_ARG(QString, messageId), Q_ARG(QString, text));
}

void AgentModule::chatReset(const QString &messageId) const {
    QMetaObject::invokeMethod(m_root, "chatReset", Q_ARG(QString, messageId));
}

// public
QHash<int, QByteArray> ConversationModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[IdRole] = "id";
    return roles;
}

#ifndef UNICOMM_GLOBALMANAGER_H
#define UNICOMM_GLOBALMANAGER_H

#include "globals.h"
#include "document/documentModule.h"
#include "service/git/gitModule.h"

class AgentModule;
class AudioService;
class LogModule;
class NuspellModule;
class PortModule;
class QNetworkAccessManager;
class ServiceModule;
class TerminalModule;
class ThreadpoolModule;
class UndoModule;
class WatchModule;

Q_MOC_INCLUDE("agent/agentModule.h")
Q_MOC_INCLUDE("analysis/nuspellModule.h")
Q_MOC_INCLUDE("core/undoModule.h")
Q_MOC_INCLUDE("debug/watchModule.h")
Q_MOC_INCLUDE("port/portModule.h")
Q_MOC_INCLUDE("runtime/threadpoolModule.h")
Q_MOC_INCLUDE("service/serviceModule.h")
Q_MOC_INCLUDE("terminal/logModule.h")
Q_MOC_INCLUDE("terminal/terminalModule.h")

class GlobalManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int theme READ themeGet CONSTANT)
    Q_PROPERTY(QString fore READ foreGet CONSTANT)
    Q_PROPERTY(QString foreHover READ foreHoverGet CONSTANT)
    Q_PROPERTY(QString forePressed READ forePressedGet CONSTANT)
    Q_PROPERTY(QString foreSelected READ foreSelectedGet CONSTANT)
    Q_PROPERTY(QString foreDisabled READ foreDisabledGet CONSTANT)

    Q_PROPERTY(QString back READ backGet CONSTANT)
    Q_PROPERTY(QString backHover READ backHoverGet CONSTANT)
    Q_PROPERTY(QString backPressed READ backPressedGet CONSTANT)
    Q_PROPERTY(QString backSelected READ backSelectedGet CONSTANT)

    Q_PROPERTY(QString stroke READ strokeGet CONSTANT)
    Q_PROPERTY(QString strokePressed READ strokePressedGet CONSTANT)

    Q_PROPERTY(QString brandFore READ brandForeGet CONSTANT)

    Q_PROPERTY(QString brandBack READ brandBackGet CONSTANT)
    Q_PROPERTY(QString brandBackSelected READ brandBackSelectedGet CONSTANT)

    Q_PROPERTY(QString brandStroke READ brandStrokeGet CONSTANT)
    Q_PROPERTY(QString brandLink READ brandLinkGet CONSTANT)

    Q_PROPERTY(QString successFore2 READ successFore2Get CONSTANT)
    Q_PROPERTY(QString successBack2 READ successBack2Get CONSTANT)
    Q_PROPERTY(QString successFore3 READ successFore3Get CONSTANT)
    Q_PROPERTY(QString successBack3 READ successBack3Get CONSTANT)

    Q_PROPERTY(QString warningFore2 READ warningFore2Get CONSTANT)
    Q_PROPERTY(QString warningBack2 READ warningBack2Get CONSTANT)
    Q_PROPERTY(QString warningFore3 READ warningFore3Get CONSTANT)
    Q_PROPERTY(QString warningBack3 READ warningBack3Get CONSTANT)

    Q_PROPERTY(QString dangerFore2 READ dangerFore2Get CONSTANT)
    Q_PROPERTY(QString dangerBack2 READ dangerBack2Get CONSTANT)
    Q_PROPERTY(QString dangerFore3 READ dangerFore3Get CONSTANT)
    Q_PROPERTY(QString dangerBack3 READ dangerBack3Get CONSTANT)

    // modules
    Q_PROPERTY(AgentModule* agentModule READ agentModuleGet CONSTANT)
    Q_PROPERTY(DocumentModule* documentModule READ documentModuleGet CONSTANT)
    Q_PROPERTY(GitModule* gitModule READ gitModuleGet CONSTANT)
    Q_PROPERTY(LogModule* logModule READ logModuleGet CONSTANT)
    Q_PROPERTY(NuspellModule* nuspellModule READ nuspellModuleGet CONSTANT)
    Q_PROPERTY(PortModule* portModule READ portModuleGet CONSTANT)
    Q_PROPERTY(ServiceModule* serviceModule READ serviceModuleGet CONSTANT)
    Q_PROPERTY(TerminalModule* terminalModule READ terminalModuleGet CONSTANT)
    Q_PROPERTY(ThreadpoolModule* threadpoolModule READ threadpoolModuleGet CONSTANT)
    Q_PROPERTY(UndoModule* undoModule READ undoModuleGet CONSTANT)
    Q_PROPERTY(WatchModule* watchModule READ watchModuleGet CONSTANT)

public:
    explicit GlobalManager(QObject *parent = nullptr);

    ~GlobalManager() override = default;

    [[nodiscard]] int themeGet() const {
        return m_theme;
    }

    [[nodiscard]] QString foreGet() const {
        return m_theme == Theme::Light ? m_palette["lightFore"] : m_palette["darkFore"];
    }

    [[nodiscard]] QString foreHoverGet() const {
        return m_theme == Theme::Light ? m_palette["lightForeHover"] : m_palette["darkForeHover"];
    }

    [[nodiscard]] QString forePressedGet() const {
        return m_theme == Theme::Light ? m_palette["lightForePressed"] : m_palette["darkForePressed"];
    }

    [[nodiscard]] QString foreSelectedGet() const {
        return m_theme == Theme::Light ? m_palette["lightForeSelected"] : m_palette["darkForeSelected"];
    }

    [[nodiscard]] QString foreDisabledGet() const {
        return m_theme == Theme::Light ? m_palette["lightForeDisabled"] : m_palette["darkForeDisabled"];
    }

    [[nodiscard]] QString backGet() const {
        return m_theme == Theme::Light ? m_palette["lightBack"] : m_palette["darkBack"];
    }

    [[nodiscard]] QString backHoverGet() const {
        return m_theme == Theme::Light ? m_palette["lightBackHover"] : m_palette["darkBackHover"];
    }

    [[nodiscard]] QString backPressedGet() const {
        return m_theme == Theme::Light ? m_palette["lightBackPressed"] : m_palette["darkBackPressed"];
    }

    [[nodiscard]] QString backSelectedGet() const {
        return m_theme == Theme::Light ? m_palette["lightBackSelected"] : m_palette["darkBackSelected"];
    }

    [[nodiscard]] QString strokeGet() const {
        return m_theme == Theme::Light ? m_palette["lightStroke"] : m_palette["darkStroke"];
    }

    [[nodiscard]] QString strokePressedGet() const {
        return m_theme == Theme::Light ? m_palette["lightStrokePressed"] : m_palette["darkStrokePressed"];
    }

    [[nodiscard]] QString brandForeGet() const {
        return m_theme == Theme::Light ? m_palette["lightBrandFore"] : m_palette["darkBrandFore"];
    }

    [[nodiscard]] QString brandBackGet() const {
        return m_theme == Theme::Light ? m_palette["lightBrandBack"] : m_palette["darkBrandBack"];
    }

    [[nodiscard]] QString brandBackSelectedGet() const {
        return m_theme == Theme::Light ? m_palette["lightBrandBackSelected"] : m_palette["darkBrandBackSelected"];
    }

    [[nodiscard]] QString brandStrokeGet() const {
        return m_theme == Theme::Light ? m_palette["lightBrandStroke"] : m_palette["darkBrandStroke"];
    }

    [[nodiscard]] QString brandLinkGet() const {
        return m_theme == Theme::Light ? m_palette["lightBrandLink"] : m_palette["darkBrandLink"];
    }

    [[nodiscard]] QString successFore2Get() const {
        return m_theme == Theme::Light ? m_palette["lightSuccessFore2"] : m_palette["darkSuccessFore2"];
    }

    [[nodiscard]] QString successBack2Get() const {
        return m_theme == Theme::Light ? m_palette["lightSuccessBack2"] : m_palette["darkSuccessBack2"];
    }

    [[nodiscard]] QString successFore3Get() const {
        return m_theme == Theme::Light ? m_palette["lightSuccessFore3"] : m_palette["darkSuccessFore3"];
    }

    [[nodiscard]] QString successBack3Get() const {
        return m_theme == Theme::Light ? m_palette["lightSuccessBack3"] : m_palette["darkSuccessBack3"];
    }

    [[nodiscard]] QString warningFore2Get() const {
        return m_theme == Theme::Light ? m_palette["lightWarningFore2"] : m_palette["darkWarningFore2"];
    }

    [[nodiscard]] QString warningBack2Get() const {
        return m_theme == Theme::Light ? m_palette["lightWarningBack2"] : m_palette["darkWarningBack2"];
    }

    [[nodiscard]] QString warningFore3Get() const {
        return m_theme == Theme::Light ? m_palette["lightWarningFore3"] : m_palette["darkWarningFore3"];
    }

    [[nodiscard]] QString warningBack3Get() const {
        return m_theme == Theme::Light ? m_palette["lightWarningBack3"] : m_palette["darkWarningBack3"];
    }

    [[nodiscard]] QString dangerFore2Get() const {
        return m_theme == Theme::Light ? m_palette["lightDangerFore2"] : m_palette["darkDangerFore2"];
    }

    [[nodiscard]] QString dangerBack2Get() const {
        return m_theme == Theme::Light ? m_palette["lightDangerBack2"] : m_palette["darkDangerBack2"];
    }

    [[nodiscard]] QString dangerFore3Get() const {
        return m_theme == Theme::Light ? m_palette["lightDangerFore3"] : m_palette["darkDangerFore3"];
    }

    [[nodiscard]] QString dangerBack3Get() const {
        return m_theme == Theme::Light ? m_palette["lightDangerBack3"] : m_palette["darkDangerBack3"];
    }

    [[nodiscard]] DocumentModule *documentModuleGet() const {
        return m_documentModule;
    }

    void documentModuleSet(DocumentModule *module) {
        m_documentModule = module;
    }

    [[nodiscard]] MainWindow *mainWindowGet() const {
        return m_mainWindow;
    }

    void mainWindowSet(MainWindow *window) {
        m_mainWindow = window;
    }

    [[nodiscard]] QNetworkAccessManager *networkAccessManagerGet() const {
        return m_networkAccessManager;
    }

    void networkAccessManagerSet(QNetworkAccessManager *manager) {
        m_networkAccessManager = manager;
    }

    [[nodiscard]] AudioService *audioServiceGet() const {
        return m_audioService;
    }

    void audioServiceSet(AudioService *service) {
        m_audioService = service;
    }

    [[nodiscard]] ServiceModule *serviceModuleGet() const {
        return m_serviceModule;
    }

    void serviceModuleSet(ServiceModule *module) {
        m_serviceModule = module;
    }

    [[nodiscard]] AgentModule *agentModuleGet() const {
        return m_agentModule;
    }

    void agentModuleSet(AgentModule *module) {
        m_agentModule = module;
    }

    [[nodiscard]] GitModule *gitModuleGet() const {
        return m_gitModule;
    }

    void gitModuleSet(GitModule *module) {
        m_gitModule = module;
    }

    [[nodiscard]] LogModule *logModuleGet() const {
        return m_logModule;
    }

    void logModuleSet(LogModule *module) {
        m_logModule = module;
    }

    [[nodiscard]] NuspellModule *nuspellModuleGet() const {
        return m_nuspellModule;
    }

    void nuspellModuleSet(NuspellModule *module) {
        m_nuspellModule = module;
    }

    [[nodiscard]] PortModule *portModuleGet() const {
        return m_portModule;
    }

    void portModuleSet(PortModule *module) {
        m_portModule = module;
    }

    [[nodiscard]] TerminalModule *terminalModuleGet() const {
        return m_terminalModule;
    }

    void terminalModuleSet(TerminalModule *module) {
        m_terminalModule = module;
    }

    [[nodiscard]] ThreadpoolModule *threadpoolModuleGet() const {
        return m_threadpoolModule;
    }

    void threadpoolModuleSet(ThreadpoolModule *module) {
        m_threadpoolModule = module;
    }

    [[nodiscard]] UndoModule *undoModuleGet() const {
        return m_undoModule;
    }

    void undoModuleSet(UndoModule *module) {
        m_undoModule = module;
    }

    [[nodiscard]] WatchModule *watchModuleGet() const {
        return m_watchModule;
    }

    void watchModuleSet(WatchModule *module) {
        m_watchModule = module;
    }

private:
    // Non-owning references; MainWindow initializes these before loading QML.
    MainWindow *m_mainWindow{};
    QNetworkAccessManager *m_networkAccessManager{};
    AudioService *m_audioService{};
    ServiceModule *m_serviceModule{};
    AgentModule *m_agentModule{};
    DocumentModule *m_documentModule{};
    GitModule *m_gitModule{};
    LogModule *m_logModule{};
    NuspellModule *m_nuspellModule{};
    PortModule *m_portModule{};
    TerminalModule *m_terminalModule{};
    ThreadpoolModule *m_threadpoolModule{};
    UndoModule *m_undoModule{};
    WatchModule *m_watchModule{};

    int m_theme{};
    QHash<QString, QString> m_palette{};
    QStringList m_styleSheet{};
};

#endif //UNICOMM_GLOBALMANAGER_H

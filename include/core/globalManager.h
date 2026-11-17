#ifndef UNICOMM_GLOBALMANAGER_H
#define UNICOMM_GLOBALMANAGER_H

#include "globals.h"

class QNetworkAccessManager;
class QQmlEngine;

class AgentModule;
class AudioService;
class DocumentModule;
class FileModule;
class GitModule;
class LogModule;
class NuspellModule;
class PortModule;
class ServiceModule;
class TerminalModule;
class ThemeManager;
class ThreadpoolModule;
class UndoModule;
class WatchModule;

Q_MOC_INCLUDE("agent/agentModule.h")
Q_MOC_INCLUDE("analysis/nuspellModule.h")
Q_MOC_INCLUDE("core/fileModule.h")
Q_MOC_INCLUDE("core/themeManager.h")
Q_MOC_INCLUDE("core/undoModule.h")
Q_MOC_INCLUDE("debug/watchModule.h")
Q_MOC_INCLUDE("document/documentModule.h")
Q_MOC_INCLUDE("port/portModule.h")
Q_MOC_INCLUDE("runtime/threadpoolModule.h")
Q_MOC_INCLUDE("service/git/gitModule.h")
Q_MOC_INCLUDE("service/serviceModule.h")
Q_MOC_INCLUDE("terminal/logModule.h")
Q_MOC_INCLUDE("terminal/terminalModule.h")

class GlobalManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(MainWindow* mainWindow READ mainWindowGet CONSTANT)
    Q_PROPERTY(ThemeManager* theme READ themeGet CONSTANT)

    // modules
    Q_PROPERTY(AgentModule* agentModule READ agentModuleGet CONSTANT)
    Q_PROPERTY(DocumentModule* documentModule READ documentModuleGet CONSTANT)
    Q_PROPERTY(FileModule* fileModule READ fileModuleGet CONSTANT)
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

    [[nodiscard]] ThemeManager *themeGet() const {
        return m_theme;
    }

    [[nodiscard]] QQmlEngine *engineGet() const {
        return m_engine;
    }

    [[nodiscard]] DocumentModule *documentModuleGet() const {
        return m_documentModule;
    }

    void documentModuleSet(DocumentModule *module) {
        m_documentModule = module;
    }

    [[nodiscard]] FileModule *fileModuleGet() const {
        return m_fileModule;
    }

    void fileModuleSet(FileModule *module) {
        m_fileModule = module;
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
    ThemeManager *m_theme{};
    QQmlEngine *m_engine{};
    // Non-owning references; MainWindow initializes these before loading QML.
    MainWindow *m_mainWindow{};
    QNetworkAccessManager *m_networkAccessManager{};
    AudioService *m_audioService{};
    ServiceModule *m_serviceModule{};
    AgentModule *m_agentModule{};
    DocumentModule *m_documentModule{};
    FileModule *m_fileModule{};
    GitModule *m_gitModule{};
    LogModule *m_logModule{};
    NuspellModule *m_nuspellModule{};
    PortModule *m_portModule{};
    TerminalModule *m_terminalModule{};
    ThreadpoolModule *m_threadpoolModule{};
    UndoModule *m_undoModule{};
    WatchModule *m_watchModule{};

};

#endif //UNICOMM_GLOBALMANAGER_H

#ifndef UNICOMM_GLOBALMANAGER_H
#define UNICOMM_GLOBALMANAGER_H

#include "globals.h"

class QNetworkAccessManager;
class QQmlEngine;

class AgentModule;
class AudioService;
class BreakpointModule;
class DebugModule;
class DiagnosticsModule;
class DocumentModule;
class ExplorerModule;
class FileModule;
class GitCommitModule;
class GitModule;
class LogModule;
class NuspellModule;
class PortModule;
class ServiceModule;
class StructureModule;
class TerminalModule;
class ThemeManager;
class ThreadpoolModule;
class ToastModule;
class UndoModule;
class WatchModule;

Q_MOC_INCLUDE("agent/agentModule.h")
Q_MOC_INCLUDE("analysis/diagnosticsModule.h")
Q_MOC_INCLUDE("analysis/nuspellModule.h")
Q_MOC_INCLUDE("analysis/structureModule.h")
Q_MOC_INCLUDE("core/explorerModule.h")
Q_MOC_INCLUDE("core/fileModule.h")
Q_MOC_INCLUDE("core/logModule.h")
Q_MOC_INCLUDE("core/undoModule.h")
Q_MOC_INCLUDE("debug/breakpointModule.h")
Q_MOC_INCLUDE("debug/debugModule.h")
Q_MOC_INCLUDE("debug/watchModule.h")
Q_MOC_INCLUDE("document/documentModule.h")
Q_MOC_INCLUDE("port/portModule.h")
Q_MOC_INCLUDE("runtime/threadpoolModule.h")
Q_MOC_INCLUDE("service/git/gitCommitModule.h")
Q_MOC_INCLUDE("service/git/gitModule.h")
Q_MOC_INCLUDE("service/serviceModule.h")
Q_MOC_INCLUDE("terminal/terminalModule.h")

class GlobalManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(MainWindow* mainWindow READ mainWindowGet CONSTANT)
    Q_PROPERTY(QJsonObject theme READ themeGet CONSTANT)

    // modules
    Q_PROPERTY(AgentModule* agentModule READ agentModuleGet CONSTANT)
    Q_PROPERTY(BreakpointModule* breakpointModule READ breakpointModuleGet CONSTANT)
    Q_PROPERTY(DebugModule* debugModule READ debugModuleGet CONSTANT)
    Q_PROPERTY(DiagnosticsModule* diagnosticsModule READ diagnosticsModuleGet CONSTANT)
    Q_PROPERTY(DocumentModule* documentModule READ documentModuleGet CONSTANT)
    Q_PROPERTY(ExplorerModule* explorerModule READ explorerModuleGet CONSTANT)
    Q_PROPERTY(FileModule* fileModule READ fileModuleGet CONSTANT)
    Q_PROPERTY(GitCommitModule* gitCommitModule READ gitCommitModuleGet CONSTANT)
    Q_PROPERTY(GitModule* gitModule READ gitModuleGet CONSTANT)
    Q_PROPERTY(LogModule* logModule READ logModuleGet CONSTANT)
    Q_PROPERTY(NuspellModule* nuspellModule READ nuspellModuleGet CONSTANT)
    Q_PROPERTY(PortModule* portModule READ portModuleGet CONSTANT)
    Q_PROPERTY(ServiceModule* serviceModule READ serviceModuleGet CONSTANT)
    Q_PROPERTY(StructureModule* structureModule READ structureModuleGet CONSTANT)
    Q_PROPERTY(TerminalModule* terminalModule READ terminalModuleGet CONSTANT)
    Q_PROPERTY(ThreadpoolModule* threadpoolModule READ threadpoolModuleGet CONSTANT)
    Q_PROPERTY(UndoModule* undoModule READ undoModuleGet CONSTANT)
    Q_PROPERTY(WatchModule* watchModule READ watchModuleGet CONSTANT)

public:
    explicit GlobalManager(QObject *parent = nullptr);

    ~GlobalManager() override = default;

    [[nodiscard]] QJsonObject themeGet() const;

    [[nodiscard]] QQmlEngine *engineGet() const {
        return m_engine;
    }

    [[nodiscard]] BreakpointModule *breakpointModuleGet() const {
        return m_breakpointModule;
    }

    void breakpointModuleSet(BreakpointModule *module) {
        m_breakpointModule = module;
    }

    [[nodiscard]] DebugModule *debugModuleGet() const {
        return m_debugModule;
    }

    void debugModuleSet(DebugModule *module) {
        m_debugModule = module;
    }

    [[nodiscard]] DiagnosticsModule *diagnosticsModuleGet() const {
        return m_diagnosticsModule;
    }

    void diagnosticsModuleSet(DiagnosticsModule *module) {
        m_diagnosticsModule = module;
    }

    [[nodiscard]] DocumentModule *documentModuleGet() const {
        return m_documentModule;
    }

    void documentModuleSet(DocumentModule *module) {
        m_documentModule = module;
    }

    [[nodiscard]] ExplorerModule *explorerModuleGet() const {
        return m_explorerModule;
    }

    void explorerModuleSet(ExplorerModule *module) {
        m_explorerModule = module;
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

    [[nodiscard]] StructureModule *structureModuleGet() const {
        return m_structureModule;
    }

    void structureModuleSet(StructureModule *module) {
        m_structureModule = module;
    }

    [[nodiscard]] AgentModule *agentModuleGet() const {
        return m_agentModule;
    }

    void agentModuleSet(AgentModule *module) {
        m_agentModule = module;
    }

    [[nodiscard]] GitCommitModule *gitCommitModuleGet() const {
        return m_gitCommitModule;
    }

    void gitCommitModuleSet(GitCommitModule *module) {
        m_gitCommitModule = module;
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

    [[nodiscard]] ToastModule *toastModuleGet() const {
        return m_toastModule;
    }

    void toastModuleSet(ToastModule *module) {
        m_toastModule = module;
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
    BreakpointModule *m_breakpointModule{};
    DebugModule *m_debugModule{};
    DiagnosticsModule *m_diagnosticsModule{};
    DocumentModule *m_documentModule{};
    ExplorerModule *m_explorerModule{};
    FileModule *m_fileModule{};
    GitCommitModule *m_gitCommitModule{};
    GitModule *m_gitModule{};
    LogModule *m_logModule{};
    NuspellModule *m_nuspellModule{};
    PortModule *m_portModule{};
    StructureModule *m_structureModule{};
    TerminalModule *m_terminalModule{};
    ThreadpoolModule *m_threadpoolModule{};
    ToastModule *m_toastModule{};
    UndoModule *m_undoModule{};
    WatchModule *m_watchModule{};

};

#endif //UNICOMM_GLOBALMANAGER_H

#ifndef UNICOMM_MAINWINDOW_H
#define UNICOMM_MAINWINDOW_H

#include <QJsonObject>
#include <kddockwidgets/qtwidgets/views/MainWindow.h>

class QQuickView;
class QShortcut;

class ConfigManager;
class GlobalManager;
class LSPManager;
class AudioService;
class Ripgrep;

class AgentModule;
class BreakpointModule;
class DebugModule;
class DiagnosticsModule;
class ExplorerModule;
class FileModule;
class GitModule;
class LogModule;
class MenuModule;
class NuspellModule;
class PortModule;
class DocumentModule;
class StatusModule;
class ToastModule;
class StructureModule;
class TerminalModule;
class ThreadpoolModule;
class WatchModule;

class UndoModule;

class MainWindow final : public KDDockWidgets::QtWidgets::MainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, const QString &uniqueName = QStringLiteral("MyMainWindow"));

    ~MainWindow() override;

    void propertySet();

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    Q_INVOKABLE void overlayFlagSet(const QVariant &transparent, const QVariant &focus);

    Q_INVOKABLE void quit();

    Q_INVOKABLE void terminate();

    Q_INVOKABLE void themeSet(int theme);

    Q_INVOKABLE void workspaceOpen();

    Q_INVOKABLE void workspaceSave(const QUrl &configUrl = QUrl());

    void quitTrack(float secondaryProgress, const QString &secondaryLog) const;

signals:
    void appendLog(int type, const QString &prefix, const QString &message);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void moduleInit();

    void shortcutInit();

    void layoutInit();

    void overlayInit();

    void overlayUpdate() const;

    void mainConfigSave();

    void maximizeToggle();

    QJsonObject m_mainConfig{};
    QQuickView *m_overlay{};
    ToastModule *m_toastModule{};
    QObject *m_closeDialog{};
    QObject *m_quitDialog{};

    ConfigManager *m_configManager{};
    LSPManager *m_lspManager{};

    AgentModule *m_agentModule{};
    BreakpointModule *m_breakpointModule{};
    DebugModule *m_debugModule{};
    DiagnosticsModule *m_diagnosticsModule{};
    ExplorerModule *m_explorerModule{};
    FileModule *m_fileModule{};
    GitModule *m_gitModule{};
    LogModule *m_logModule{};
    MenuModule *m_menuModule{};
    NuspellModule *m_nuspellModule{};
    PortModule *m_portModule{};
    DocumentModule *m_documentModule{};
    StatusModule *m_statusModule{};
    StructureModule *m_structureModule{};
    TerminalModule *m_terminalModule{};
    ThreadpoolModule *m_threadpoolModule{};
    UndoModule *m_undoModule{};
    WatchModule *m_watchModule{};
};

#endif //UNICOMM_MAINWINDOW_H

#include "mainWindow/mainWindow.h"

#include <QCameraDevice>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <kddockwidgets/LayoutSaver.h>
#include <kddockwidgets/core/DockRegistry.h>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <kddockwidgets/qtwidgets/views/MainWindow.h>

#include "globals.h"
#include "agent/agentModule.h"
#include "analysis/diagnosticsModule.h"
#include "analysis/nuspellModule.h"
#include "analysis/structureModule.h"
#include "core/configManager.h"
#include "core/explorerModule.h"
#include "core/fileModule.h"
#include "core/globalManager.h"
#include "core/undoModule.h"
#include "debug/breakpointModule.h"
#include "debug/debugModule.h"
#include "debug/watchModule.h"
#include "document/documentModule.h"

#include "mainWindow/menuModule.h"
#include "mainWindow/statusModule.h"
#include "mainWindow/toastModule.h"
#include "port/portModule.h"
#include "runtime/luaInterpreter.h"
#include "runtime/threadpoolModule.h"
#include "service/audio.h"
#include "service/ripgrep.h"
#include "service/git/gitModule.h"
#include "service/lsp/lspManager.h"
#include "terminal/logModule.h"
#include "terminal/terminalModule.h"

// public
MainWindow::MainWindow(QWidget *parent, const QString &uniqueName)
    : KDDockWidgets::QtWidgets::MainWindow(uniqueName, KDDockWidgets::MainWindowOption_None, parent) {
    // mainWindow ui init
    g_mainWindow = this;
    setAttribute(Qt::WA_DeleteOnClose);
    QWidget::setWindowTitle("UniComm");
    QWidget::setWindowIcon(QIcon(":/icon/icon.ico"));

    moduleInit();
    shortcutInit();
    overlayInit();
    layoutInit();

    // logging
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] main window created").arg(timestamp);

    // preload multimedia to avoid lagging on port selection
    QThread *worker = QThread::create([] {
        QMediaDevices::videoInputs();
    });
    worker->start();
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
}

MainWindow::~MainWindow() {
    delete m_toastModule;

    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] main window destructed").arg(timestamp);
}

void MainWindow::propertySet() {
    m_overlay->rootContext()->setContextProperty("mainWindow", this);
    m_overlay->rootContext()->setContextProperty("global", g_globalManager);
    m_overlay->rootContext()->setContextProperty("agentModule", m_agentModule);
    m_overlay->rootContext()->setContextProperty("breakpointModule", m_breakpointModule);
    m_overlay->rootContext()->setContextProperty("debugModule", m_debugModule);
    m_overlay->rootContext()->setContextProperty("diagnosticsModule", m_diagnosticsModule);
    m_overlay->rootContext()->setContextProperty("documentModule", m_documentModule);
    m_overlay->rootContext()->setContextProperty("explorerModule", m_explorerModule);
    m_overlay->rootContext()->setContextProperty("gitModule", m_gitModule);
    m_overlay->rootContext()->setContextProperty("logModule", m_logModule);
    m_overlay->rootContext()->setContextProperty("menuModule", m_menuModule);
    m_overlay->rootContext()->setContextProperty("portModule", m_portModule);
    m_overlay->rootContext()->setContextProperty("statusModule", m_statusModule);
    // m_overlay->rootContext()->setContextProperty("structureModule", m_structureModule);
    m_overlay->rootContext()->setContextProperty("fileModule", m_fileModule);
    m_overlay->rootContext()->setContextProperty("terminalModule", m_terminalModule);
    m_overlay->rootContext()->setContextProperty("threadpoolModule", m_threadpoolModule);
    m_overlay->rootContext()->setContextProperty("undoModule", m_undoModule);
    m_overlay->rootContext()->setContextProperty("watchModule", m_watchModule);

    m_overlay->rootContext()->setContextProperty("agentModuleAction", QVariant::fromValue(m_agentModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("breakpointModuleAction", QVariant::fromValue(m_breakpointModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("debugModuleAction", QVariant::fromValue(m_debugModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("diagnosticsModuleAction", QVariant::fromValue(m_diagnosticsModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("explorerModuleAction", QVariant::fromValue(m_explorerModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("portModuleAction", QVariant::fromValue(m_portModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("structureModuleAction", QVariant::fromValue(m_structureModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("threadpoolModuleAction", QVariant::fromValue(m_threadpoolModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("watchModuleAction", QVariant::fromValue(m_watchModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("logModuleAction", QVariant::fromValue(m_logModule->toggleAction()));
    m_overlay->rootContext()->setContextProperty("gitModuleAction", QVariant::fromValue(m_gitModule->toggleAction()));
}

void MainWindow::propertyGet(const QVariantMap &objects) {
    m_closeDialog = qvariant_cast<QObject *>(objects["mainWindowCloseDialog"]);
    m_quitDialog = qvariant_cast<QObject *>(objects["mainWindowQuitDialog"]);

    const QVariantHash agentObjects = {
        {"mainWindowLinkMenu", objects["mainWindowLinkMenu"]},
        {"mainWindowToast", QVariant::fromValue(m_toastModule)},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"documentModule", QVariant::fromValue(m_documentModule)},
        {"fileModule", QVariant::fromValue(m_fileModule)},
        {"agentModuleRenameDialog", objects["agentModuleRenameDialog"]},
        {"agentModuleModeMenu", objects["agentModuleModeMenu"]},
        {"agentModuleModelMenu", objects["agentModuleModelMenu"]}
    };
    m_agentModule->propertySet(agentObjects);

    const QVariantHash breakpointObjects = {
        {"breakpointModuleLineMenu", objects["breakpointModuleLineMenu"]},
        {"breakpointModuleFileMenu", objects["breakpointModuleFileMenu"]},
        {"breakpointModuleRootMenu", objects["breakpointModuleRootMenu"]}
    };
    m_breakpointModule->propertySet(breakpointObjects);

    const QVariantHash debugObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"debugModuleErrorDialog", objects["debugModuleErrorDialog"]}
    };
    m_debugModule->propertySet(debugObjects);

    const QVariantHash diagnosticsObjects = {
        {"diagnosticsModuleDiagnosticMenu", objects["diagnosticsModuleDiagnosticMenu"]}
    };
    m_diagnosticsModule->propertySet(diagnosticsObjects);

    const QVariantHash documentObjects = {
        {"mainWindowToast", QVariant::fromValue(m_toastModule)},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"breakpointModuleEditDialog", objects["breakpointModuleEditDialog"]},
        {"fileModulePropertyDialog", objects["fileModulePropertyDialog"]},
        {"documentModuleGotoDialog", objects["documentModuleGotoDialog"]},
        {"documentModuleRenameDialog", objects["documentModuleRenameDialog"]},
        {"documentModuleSaveDialog", objects["documentModuleSaveDialog"]},
        {"documentModuleEditorMenu", objects["documentModuleEditorMenu"]},
        {"documentModuleCompletionToolTip", objects["documentModuleCompletionToolTip"]},
        {"documentModuleCompletionTableView", objects["documentModuleCompletionTableView"]},
        {"documentModuleCompletionDetailTableView", objects["documentModuleCompletionDetailTableView"]},
        {"documentModuleDwellToolTip", objects["documentModuleDwellToolTip"]},
        {"documentModuleDwellDiagnosticTextArea", objects["documentModuleDwellDiagnosticTextArea"]},
        {"documentModuleDwellHoverTextArea", objects["documentModuleDwellHoverTextArea"]},
        {"documentModuleDwellCodeActionMenu", objects["documentModuleDwellCodeActionMenu"]},
        {"documentModuleDwellSuggestionMenu", objects["documentModuleDwellSuggestionMenu"]},
        {"documentModuleNavigationToolTip", objects["documentModuleNavigationToolTip"]},
        {"documentModuleNavigationTableView", objects["documentModuleNavigationTableView"]},
        {"documentModuleNavigationDetailLabel", objects["documentModuleNavigationDetailLabel"]},
        {"documentModulePositionTooltip", objects["documentModulePositionTooltip"]},
        {"documentModuleSignatureToolTip", objects["documentModuleSignatureToolTip"]},
        {"documentModuleSignatureLabel", objects["documentModuleSignatureLabel"]}
    };
    m_documentModule->propertySet(documentObjects);

    const QVariantHash explorerObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"explorerModuleFileMenu", objects["explorerModuleFileMenu"]},
        {"explorerModuleFolderMenu", objects["explorerModuleFolderMenu"]},
        {"explorerModuleRootMenu", objects["explorerModuleRootMenu"]}
    };
    m_explorerModule->propertySet(explorerObjects);

    const QVariantHash gitObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"gitModuleContinueDialog", objects["gitModuleContinueDialog"]},
        {"gitModuleErrorDialog", objects["gitModuleErrorDialog"]},
        {"gitModuleProxyDialog", objects["gitModuleProxyDialog"]},
        {"gitModuleRemoteAddDialog", objects["gitModuleRemoteAddDialog"]},
        {"gitModuleBranchMenu", objects["gitModuleBranchMenu"]},
        {"gitModuleLogMenu", objects["gitModuleLogMenu"]}
    };
    m_gitModule->propertySet(gitObjects);

    const QVariantHash logObjects = {
        {"mainWindowLinkMenu", objects["mainWindowLinkMenu"]},
        {"mainWindowTextView", objects["mainWindowTextView"]},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"logModuleHeightDialog", objects["logModuleHeightDialog"]},
        {"mainWindowToast", QVariant::fromValue(m_toastModule)}
    };
    m_logModule->propertySet(logObjects);

    const QVariantHash menuObjects = {
        {"menuModuleFileMenu", objects["menuModuleFileMenu"]},
        {"menuModuleEditMenu", objects["menuModuleEditMenu"]},
        {"menuModuleViewMenu", objects["menuModuleViewMenu"]},
        {"menuModuleNavMenu", objects["menuModuleNavMenu"]},
        {"menuModuleCodeMenu", objects["menuModuleCodeMenu"]},
        {"menuModuleExecMenu", objects["menuModuleExecMenu"]},
        {"menuModuleGitMenu", objects["menuModuleGitMenu"]}
    };
    m_menuModule->propertySet(menuObjects);

    const QVariantHash portObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"portModuleTableMenu", objects["portModuleTableMenu"]},
        {"portModuleRootMenu", objects["portModuleRootMenu"]}
    };
    m_portModule->propertySet(portObjects);

    const QVariantHash statusObjects = {
        {"statusModuleEolModeMenu", objects["statusModuleEolModeMenu"]},
        {"statusModuleBackgroundToolTip", objects["statusModuleBackgroundToolTip"]},
        {"statusModuleThreadToolTip", objects["statusModuleThreadToolTip"]}
    };
    m_statusModule->propertySet(statusObjects);

    const QVariantHash structureObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"structureModuleRootMenu", objects["structureModuleRootMenu"]}
    };
    m_structureModule->propertySet(structureObjects);

    const QVariantHash fileObjects = {
        {"mainWindowToast", QVariant::fromValue(m_toastModule)}
    };
    m_fileModule->propertySet(fileObjects);

    const QVariantHash terminalObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"terminalModuleContextMenu", objects["terminalModuleContextMenu"]},
        {"terminalModuleTerminalMenu", objects["terminalModuleTerminalMenu"]}
    };
    m_terminalModule->propertySet(terminalObjects);

    const QVariantHash threadpoolObjects = {
        {"threadpoolModuleErrorDialog", objects["threadpoolModuleErrorDialog"]},
        {"threadpoolModuleThreadMenu", objects["threadpoolModuleThreadMenu"]}
    };
    m_threadpoolModule->propertySet(threadpoolObjects);

    const QVariantHash watchObjects = {
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"watchModuleExpressionMenu", objects["watchModuleExpressionMenu"]},
        {"watchModuleValueMenu", objects["watchModuleValueMenu"]},
        {"watchModuleRootMenu", objects["watchModuleRootMenu"]}
    };
    m_watchModule->propertySet(watchObjects);
}

void MainWindow::overlayFlagSet(const QVariant &transparent, const QVariant &focus) {
    if (transparent.isValid()) {
        const auto _transparent = transparent.toBool();
        if (m_overlay->flags().testFlag(Qt::WindowTransparentForInput) != _transparent) {
            m_overlay->setFlag(Qt::WindowTransparentForInput, _transparent);
        }
    }
    if (focus.isValid()) {
        const auto _focus = focus.toBool();
        if (!_focus && !isActiveWindow()) activateWindow();
        if (m_overlay->flags().testFlag(Qt::WindowDoesNotAcceptFocus) == _focus) {
            m_overlay->setFlag(Qt::WindowDoesNotAcceptFocus, !_focus);
        }
    }
    m_overlay->hide();
    m_overlay->show();
}

void MainWindow::quit() {
    workspaceSave();

    QMetaObject::invokeMethod(m_quitDialog, "open");
    constexpr float total = 2;
    int current = 0;
    // quit modules
    current++;
    m_quitDialog->setProperty("primaryLog", tr("Waiting for threadpool module..."));
    m_quitDialog->setProperty("primaryProgress", static_cast<float>(current) / total);
    m_threadpoolModule->quit();
    current++;
    m_quitDialog->setProperty("primaryLog", tr("Waiting for lua language server module..."));
    m_quitDialog->setProperty("primaryProgress", static_cast<float>(current) / total);
    m_lspManager->shutdown();

    terminate();
}

void MainWindow::terminate() {
    g_terminating = true;
    KDDockWidgets::DockRegistry::self()->clear();
    delete m_portModule;
    m_portModule = nullptr;
    g_port = nullptr;
    g_portModel = nullptr;
    close();
}

void MainWindow::themeSet(const int theme) {
    workspaceSave();
    // write to main config
    g_mainConfig["theme"] = theme;
    const auto jsonDoc = QJsonDocument(g_mainConfig);
    auto mainConfig = QFile(QDir::current().filePath("config.json"));
    if (!mainConfig.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    const auto jsonData = jsonDoc.toJson(QJsonDocument::Indented);
    mainConfig.write(jsonData);
    mainConfig.close();
    // restart main process
    QProcess::startDetached(QCoreApplication::applicationFilePath());
    terminate();
}

void MainWindow::workspaceOpen() {
    // ask for new workspace location
    const auto workspaceDir = QFileDialog::getExistingDirectory(
        g_mainWindow,
        tr("Open Workspace"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (workspaceDir.isEmpty()) return;
    const auto workspaceUrl = QUrl::fromLocalFile(workspaceDir);
    if (g_workspaceUrl == workspaceUrl) {
        qDebug() << "same as prev workspace";
        return;
    }
    workspaceSave();
    // write to main config
    g_mainConfig["workspace"] = workspaceUrl.toString();
    const auto jsonDoc = QJsonDocument(g_mainConfig);
    auto mainConfig = QFile(QDir::current().filePath("config.json"));
    if (!mainConfig.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    const auto jsonData = jsonDoc.toJson(QJsonDocument::Indented);
    mainConfig.write(jsonData);
    mainConfig.close();
    // restart main process
    QProcess::startDetached(QCoreApplication::applicationFilePath());
    terminate();
}

void MainWindow::workspaceSave(const QUrl &configUrl) {
    m_agentModule->agentConfigSave();
    BreakpointModule::breakpointConfigSave();
    m_documentModule->documentConfigSave();
    m_logModule->logConfigSave();
    m_portModule->portConfigSave();
    m_terminalModule->terminalConfigSave();
    WatchModule::watchConfigSave();
    mainConfigSave();
    m_configManager->workspaceConfigSave(configUrl);

    m_explorerModule->indexUpdate();
}

void MainWindow::quitTrack(const float secondaryProgress, const QString &secondaryLog) const {
    m_quitDialog->setProperty("secondaryProgress", secondaryProgress);
    m_quitDialog->setProperty("secondaryLog", secondaryLog);
}

// protected
void MainWindow::closeEvent(QCloseEvent *event) {
    if (g_terminating) {
        event->accept();
    } else {
        event->ignore();
        QMetaObject::invokeMethod(m_closeDialog, "open");
    }
}

// private
void MainWindow::moduleInit() {
    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2").arg(timestamp, "initializing module");

    g_networkAccessManager = new QNetworkAccessManager(qApp);
    m_configManager = new ConfigManager(this);
    m_lspManager = new LSPManager(this);
    g_audioService = new AudioService(this);
    g_ripgrep = new Ripgrep(this);
    g_globalManager = new GlobalManager();

    m_breakpointModule = new BreakpointModule();
    m_debugModule = new DebugModule();
    m_diagnosticsModule = new DiagnosticsModule();
    m_documentModule = new DocumentModule(this);
    m_explorerModule = new ExplorerModule();
    m_fileModule = new FileModule();
    m_gitModule = new GitModule();
    m_agentModule = new AgentModule();
    m_logModule = new LogModule();
    m_menuModule = new MenuModule(this);
    m_nuspellModule = new NuspellModule(this);
    m_portModule = new PortModule();
    m_statusModule = new StatusModule(this);
    m_structureModule = new StructureModule();
    m_terminalModule = new TerminalModule(this);
    m_threadpoolModule = new ThreadpoolModule();
    m_undoModule = new UndoModule(this);
    m_watchModule = new WatchModule();

    m_mainConfig = g_workspaceConfig["mainConfig"].toObject();
    g_agent = m_agentModule;
    g_document = m_documentModule;
    g_git = m_gitModule;
    g_log = m_logModule;
    g_nuspell = m_nuspellModule;
    g_port = m_portModule;
    g_terminal = m_terminalModule;
    g_threadpool = m_threadpoolModule;
    g_undo = m_undoModule;

    connect(this, &MainWindow::appendLog, m_logModule, &LogModule::logAppend);

    connect(m_configManager, &ConfigManager::appendLog, m_logModule, &LogModule::logAppend);

    connect(m_lspManager, &LSPManager::notificationDiagnostics, m_documentModule, &DocumentModule::diagnosticsNotification);
    connect(m_lspManager, &LSPManager::notificationDiagnostics, m_diagnosticsModule, &DiagnosticsModule::diagnosticsNotification);
    connect(m_lspManager, &LSPManager::responseCodeAction, m_documentModule, &DocumentModule::responseCodeAction);
    connect(m_lspManager, &LSPManager::responseCompletion, m_documentModule, &DocumentModule::completionResponse);
    connect(m_lspManager, &LSPManager::responseDefinition, m_documentModule, &DocumentModule::definitionResponse);
    connect(m_lspManager, &LSPManager::responseDocumentHighlight, m_documentModule, &DocumentModule::documentHighlightResponse);
    connect(m_lspManager, &LSPManager::responseDocumentSymbol, m_documentModule, &DocumentModule::documentSymbolResponse);
    connect(m_lspManager, &LSPManager::responseDocumentSymbol, m_structureModule, &StructureModule::documentSymbolResponse);
    connect(m_lspManager, &LSPManager::responseFoldingRange, m_documentModule, &DocumentModule::foldingRangeResponse);
    connect(m_lspManager, &LSPManager::responseFormatting, m_documentModule, &DocumentModule::formattingResponse);
    connect(m_lspManager, &LSPManager::responseHover, m_documentModule, &DocumentModule::hoverResponse);
    connect(m_lspManager, &LSPManager::responseImplementation, m_documentModule, &DocumentModule::implementationResponse);
    connect(m_lspManager, &LSPManager::responseOnTypeFormatting, m_documentModule, &DocumentModule::onTypeFormattingResponse);
    connect(m_lspManager, &LSPManager::responsePrepareRename, m_documentModule, &DocumentModule::prepareRenameResponse);
    connect(m_lspManager, &LSPManager::responseRangeFormatting, m_documentModule, &DocumentModule::rangeFormattingResponse);
    connect(m_lspManager, &LSPManager::responseReferences, m_documentModule, &DocumentModule::referencesResponse);
    connect(m_lspManager, &LSPManager::responseRename, m_documentModule, &DocumentModule::renameResponse);
    connect(m_lspManager, &LSPManager::responseSemanticTokens, m_documentModule, &DocumentModule::semanticTokensResponse);
    connect(m_lspManager, &LSPManager::responseSignatureHelp, m_documentModule, &DocumentModule::signatureHelpResponse);
    connect(m_lspManager, &LSPManager::responseTypeDefinition, m_documentModule, &DocumentModule::typeDefinitionResponse);

    connect(m_breakpointModule, &BreakpointModule::openDocument, m_documentModule, &DocumentModule::documentOpen);
    connect(m_breakpointModule, &BreakpointModule::addMarker, m_documentModule, &DocumentModule::markerAdd);
    connect(m_breakpointModule, &BreakpointModule::deleteMarker, m_documentModule, &DocumentModule::markerDelete);

    connect(m_debugModule, &DebugModule::getIndex, m_documentModule, &DocumentModule::indexGet);
    connect(m_debugModule, &DebugModule::addMarker, m_documentModule, &DocumentModule::markerAdd);
    connect(m_debugModule, &DebugModule::setState, m_threadpoolModule, &ThreadpoolModule::stateSet);

    connect(m_diagnosticsModule, &DiagnosticsModule::setIndex, m_documentModule, &DocumentModule::indexSet);
    connect(m_diagnosticsModule, &DiagnosticsModule::fillIndicator, m_documentModule, &DocumentModule::indicatorFill);

    connect(m_documentModule, &DocumentModule::requestJson, m_lspManager, &LSPManager::jsonRequest);
    connect(m_documentModule, &DocumentModule::notificationJson, m_lspManager, &LSPManager::jsonNotification);
    connect(m_documentModule, &DocumentModule::requestSpellCheck, m_nuspellModule, &NuspellModule::spellCheckRequest);
    connect(m_documentModule, &DocumentModule::appendLog, m_logModule, &LogModule::logAppend);
    connect(m_documentModule, &DocumentModule::updateDiff, m_agentModule, &AgentModule::diffUpdate);
    connect(m_documentModule, &DocumentModule::openWorkspace, this, &MainWindow::workspaceOpen);
    connect(m_documentModule, &DocumentModule::focusDocument, m_statusModule, &StatusModule::documentFocus);
    connect(m_documentModule, &DocumentModule::focusDocument, m_structureModule, &StructureModule::documentFocus);
    connect(m_documentModule, &DocumentModule::changeSelection, m_statusModule, &StatusModule::selectionChange);
    connect(m_documentModule, &DocumentModule::insertBreakpoint, m_breakpointModule, &BreakpointModule::breakpointInsert);
    connect(m_documentModule, &DocumentModule::removeBreakpoint, m_breakpointModule, &BreakpointModule::breakpointRemove);

    connect(m_explorerModule, &ExplorerModule::appendLog, m_logModule, &LogModule::logAppend);
    connect(m_explorerModule, &ExplorerModule::openDocument, m_documentModule, &DocumentModule::documentOpen);
    connect(m_explorerModule, &ExplorerModule::startThread, m_threadpoolModule, &ThreadpoolModule::threadStart);

    connect(m_logModule, &LogModule::openFileInExplorer, m_fileModule, &FileModule::fileOpenInExplorer);
    connect(m_logModule, &LogModule::openFileInApplication, m_fileModule, &FileModule::fileOpenInApplication);

    connect(m_fileModule, &FileModule::setPermission, m_documentModule, &DocumentModule::permissionSet);
    connect(m_fileModule, &FileModule::appendBackground, m_statusModule, &StatusModule::backgroundAppend);
    connect(m_fileModule, &FileModule::removeBackground, m_statusModule, &StatusModule::backgroundRemove);
    connect(m_fileModule, &FileModule::refreshBackground, m_statusModule, &StatusModule::backgroundRefresh);

    connect(m_gitModule, &GitModule::updateIndex, m_explorerModule, &ExplorerModule::indexUpdate);
    connect(m_gitModule, &GitModule::openDocument, m_documentModule, &DocumentModule::documentOpen);
    connect(m_gitModule, &GitModule::appendBackground, m_statusModule, &StatusModule::backgroundAppend);
    connect(m_gitModule, &GitModule::removeBackground, m_statusModule, &StatusModule::backgroundRemove);
    connect(m_gitModule, &GitModule::refreshBackground, m_statusModule, &StatusModule::backgroundRefresh);

    connect(m_menuModule, &MenuModule::setTheme, this, &MainWindow::themeSet);

    connect(m_nuspellModule, &NuspellModule::responseSpellCheck, m_documentModule, &DocumentModule::spellCheckResponse);

    connect(m_portModule, &PortModule::appendLog, m_logModule, &LogModule::logAppend);

    connect(m_statusModule, &StatusModule::gotoDocument, m_documentModule, &DocumentModule::documentGoto);

    connect(m_structureModule, &StructureModule::appendLog, m_logModule, &LogModule::logAppend);
    connect(m_structureModule, &StructureModule::setFocus, m_documentModule, &DocumentModule::focusSet);
    connect(m_structureModule, &StructureModule::setIndex, m_documentModule, &DocumentModule::indexSet);
    connect(m_structureModule, &StructureModule::addMarker, m_documentModule, &DocumentModule::markerAdd);

    connect(m_threadpoolModule, &ThreadpoolModule::trackQuit, this, &MainWindow::quitTrack);
    connect(m_threadpoolModule, &ThreadpoolModule::refreshThread, m_statusModule, &StatusModule::threadRefresh);
    connect(m_threadpoolModule, &ThreadpoolModule::openDocument, m_documentModule, &DocumentModule::documentOpen);
    connect(m_threadpoolModule, &ThreadpoolModule::addMarker, m_documentModule, &DocumentModule::markerAdd);
    connect(m_threadpoolModule, &ThreadpoolModule::deleteMarker, m_documentModule, &DocumentModule::markerDelete);
    connect(m_threadpoolModule, &ThreadpoolModule::insertCallStack, m_debugModule, &DebugModule::callStackInsert);
    connect(m_threadpoolModule, &ThreadpoolModule::startDebug, m_debugModule, &DebugModule::debugStart);
    connect(m_threadpoolModule, &ThreadpoolModule::stopDebug, m_debugModule, &DebugModule::debugStop);
    connect(m_threadpoolModule, &ThreadpoolModule::openTerminal, m_terminalModule, &TerminalModule::terminalOpen);
    connect(m_threadpoolModule, &ThreadpoolModule::closeTerminal, m_terminalModule, &TerminalModule::terminalClose);
    connect(m_threadpoolModule, &ThreadpoolModule::writeTerminal, m_terminalModule, &TerminalModule::terminalWrite);
    connect(m_threadpoolModule, &ThreadpoolModule::appendLog, m_logModule, &LogModule::logAppend);

    connect(m_undoModule, &UndoModule::updateChange, m_agentModule, &AgentModule::changeUpdate);
}

void MainWindow::shortcutInit() {
    auto shortcutConfig = g_workspaceConfig["shortcutConfig"].toObject();
    const auto *maximizeShortcut = new QShortcut(QKeySequence("F11"), this); // NOLINT
    connect(maximizeShortcut, &QShortcut::activated, this, &MainWindow::maximizeToggle);
    const auto *undoShortcut = new QShortcut(QKeySequence("Ctrl+Z"), this); // NOLINT
    connect(undoShortcut, &QShortcut::activated, this, [this] {
        if (m_undoModule->canUndo()) {
            m_undoModule->undo();
        } else {
            m_toastModule->show(ToastLevel::Info, tr("Undo"), tr("Nothing to undo."));
        }
    });
    const auto *redoShortcut = new QShortcut(QKeySequence("Ctrl+Y"), this); // NOLINT
    connect(redoShortcut, &QShortcut::activated, this, [this] {
        if (m_undoModule->canRedo()) {
            m_undoModule->redo();
        } else {
            m_toastModule->show(ToastLevel::Info, tr("Redo"), tr("Nothing to redo."));
        }
    });
    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2").arg(timestamp, "shortcut initialized");
}

// // setting menu
// {
//     auto *settingAction = new QAction(tr("Setting"), this); // NOLINT
//     toolBar->addAction(settingAction);
//     connect(settingAction, &QAction::triggered, this, [this] {
//         const QJsonObject logConfig = g_workspaceConfig["logConfig"].toObject();
//         const QJsonObject documentConfig = g_workspaceConfig["documentConfig"].toObject();
//         const QJsonObject settingConfig = {
//             {"fontFamilyLog", logConfig["fontFamily"].toString()},
//             {"fontSizeLog", logConfig["fontSize"].toInt()},
//             {"fontFamilyScript", documentConfig["fontFamily"].toString()},
//             {"fontSizeScript", documentConfig["fontSize"].toInt()},
//             {"indicatorErrorStyleScript", documentConfig["indicatorErrorStyle"].toInt()},
//             {"indicatorErrorColorScript", documentConfig["indicatorErrorColor"].toString()},
//             {"indicatorWarningStyleScript", documentConfig["indicatorWarningStyle"].toInt()},
//             {"indicatorWarningColorScript", documentConfig["indicatorWarningColor"].toString()},
//             {"indicatorInfoStyleScript", documentConfig["indicatorInfoStyle"].toInt()},
//             {"indicatorInfoColorScript", documentConfig["indicatorInfoColor"].toString()},
//             {"indicatorHintStyleScript", documentConfig["indicatorHintStyle"].toInt()},
//             {"indicatorHintColorScript", documentConfig["indicatorHintColor"].toString()},
//             {"indicatorHighlightStyleScript", documentConfig["indicatorHighlightStyle"].toInt()},
//             {"indicatorHighlightColorScript", documentConfig["indicatorHighlightColor"].toString()},
//             {"indicatorReadStyleScript", documentConfig["indicatorReadStyle"].toInt()},
//             {"indicatorReadColorScript", documentConfig["indicatorReadColor"].toString()},
//             {"indicatorWriteStyleScript", documentConfig["indicatorWriteStyle"].toInt()},
//             {"indicatorWriteColorScript", documentConfig["indicatorWriteColor"].toString()},
//             {"indicatorSearchStyleScript", documentConfig["indicatorSearchStyle"].toInt()},
//             {"indicatorSearchColorScript", documentConfig["indicatorSearchColor"].toString()},
//             {"indicatorSelectionStyleScript", documentConfig["indicatorSelectionStyle"].toInt()},
//             {"indicatorSelectionColorScript", documentConfig["indicatorSelectionColor"].toString()},
//             {"indicatorHyperlinkStyleScript", documentConfig["indicatorHyperlinkStyle"].toInt()},
//             {"indicatorHyperlinkColorScript", documentConfig["indicatorHyperlinkColor"].toString()},
//             {"markerBreakpointStyleScript", documentConfig["markerBreakpointStyle"].toInt()},
//             {"markerBreakpointBackgroundScript", documentConfig["markerBreakpointBackground"].toString()},
//             {"markerBreakpointForegroundScript", documentConfig["markerBreakpointForeground"].toString()},
//             {"markerDebugStyleScript", documentConfig["markerDebugStyle"].toInt()},
//             {"markerDebugBackgroundScript", documentConfig["markerDebugBackground"].toString()},
//             {"markerDebugForegroundScript", documentConfig["markerDebugForeground"].toString()}
//         };
//         m_settingModule->settingImport(settingConfig);
//         if (m_settingModule->exec() == QDialog::Accepted) {
//             workspaceSave();
//         }
//     });
// }
// // logging
// QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
// qDebug() << QString("[%1] %2").arg(timestamp, "menu initialized");

void MainWindow::layoutInit() {
    auto *toolBar = new QToolBar(); // NOLINT
    addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->addWidget(m_menuModule);
    toolBar->setMovable(false);

    auto *statusBar = this->statusBar();
    statusBar->addWidget(m_statusModule, 1);

    bool restored = false;
    if (!m_mainConfig["state"].toString().isEmpty()) {
        const QByteArray layoutData = QByteArray::fromBase64(m_mainConfig["state"].toString().toLatin1());
        KDDockWidgets::LayoutSaver layoutSaver;
        restored = layoutSaver.restoreLayout(layoutData);
    }
    if (!restored) {
        showMaximized();

        addDockWidget(m_documentModule->welcomePage(), KDDockWidgets::Location_OnRight);
        // left
        addDockWidget(m_portModule, KDDockWidgets::Location_OnLeft, nullptr, KDDockWidgets::InitialOption(KDDockWidgets::Size(600, 0)));
        addDockWidget(m_explorerModule, KDDockWidgets::Location_OnBottom, m_portModule);
        addDockWidget(m_structureModule, KDDockWidgets::Location_OnBottom, m_explorerModule);
        // right
        addDockWidget(m_agentModule, KDDockWidgets::Location_OnRight, nullptr, KDDockWidgets::InitialOption(KDDockWidgets::Size(800, 0)));
        // bottom
        addDockWidget(m_logModule, KDDockWidgets::Location_OnBottom, nullptr, KDDockWidgets::InitialOption(KDDockWidgets::Size(0, 200)));
        m_logModule->addDockWidgetAsTab(m_gitModule, KDDockWidgets::InitialVisibilityOption::PreserveCurrentTab);
        m_logModule->addDockWidgetAsTab(m_diagnosticsModule, KDDockWidgets::InitialVisibilityOption::PreserveCurrentTab);
        addDockWidget(m_threadpoolModule, KDDockWidgets::Location_OnRight, m_logModule, KDDockWidgets::InitialOption(KDDockWidgets::Size(800, 0)));
    }
    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2").arg(timestamp, "layout initialized");
}

void MainWindow::overlayInit() {
    m_overlay = new QQuickView();
    m_overlay->setResizeMode(QQuickView::SizeRootObjectToView);
    m_overlay->setColor(Qt::transparent);
    m_overlay->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    m_overlay->setTransientParent(windowHandle());
    propertySet();

    auto *owner = windowHandle();
    Q_ASSERT(owner);
    m_toastModule = new ToastModule(m_overlay->engine(), *owner);

    m_overlay->setSource(QUrl("qrc:/qml/mainWindow/mainWindow.qml"));

    overlayUpdate();
    m_overlay->show();
    QTimer::singleShot(0, m_overlay, [this] { overlayUpdate(); });
    connect(windowHandle(), &QWindow::screenChanged, m_overlay, [this] { overlayUpdate(); });
    connect(windowHandle(), &QWindow::visibilityChanged, m_overlay, [this](const QWindow::Visibility visible) {
        if (visible == QWindow::Minimized || visible == QWindow::Hidden) m_overlay->hide();
        else m_overlay->show();
    });
}

void MainWindow::overlayUpdate() const {
    const auto screens = QGuiApplication::screens();
    QRect geometry{};
    for (const auto *screen: screens) {
        geometry = geometry.united(screen->geometry());
    }
    auto *screen = windowHandle()->screen();
    m_overlay->setScreen(screen);
    m_overlay->setGeometry(geometry);
    const QRect mainGeometry(
        m_overlay->mapFromGlobal(screen->geometry().topLeft()),
        screen->geometry().size()
    );
    m_overlay->rootObject()->setProperty("mainGeometry", mainGeometry);
}

void MainWindow::mainConfigSave() {
    const KDDockWidgets::LayoutSaver layoutSaver{};
    const QByteArray layoutData = layoutSaver.serializeLayout();
    m_mainConfig["state"] = QString(layoutData.toBase64());
    g_workspaceConfig["mainConfig"] = m_mainConfig;
}

void MainWindow::maximizeToggle() {
    if (isMaximized()) showNormal();
    else showMaximized();
}

#include "globals.h"

MainWindow *g_mainWindow = nullptr;
QNetworkAccessManager *g_networkAccessManager = nullptr;
GlobalManager *g_globalManager = nullptr;
AgentModule *g_agent = nullptr;
AudioService *g_audioService = nullptr;
Ripgrep *g_ripgrep = nullptr;
DocumentModule *g_document = nullptr;
GitModule *g_git = nullptr;
LogModule *g_log = nullptr;
NuspellModule *g_nuspell = nullptr;
PortModule *g_port = nullptr;
TerminalModule *g_terminal = nullptr;
ThreadpoolModule *g_threadpool = nullptr;
UndoModule *g_undo = nullptr;

bool g_terminating = false;
int g_theme = {};
QUrl g_workspaceUrl = {};
QString g_gitRootPath = {};
QJsonObject g_mainConfig = {};
QJsonObject g_workspaceConfig = {};

ViSession g_rm = VI_NULL;

QVariantMap g_cursorPosition = {
    {"url", QUrl()},
    {"line", -1},
    {"character", -1}
};

QHash<QChar, int> g_gitStatusCode = {
    {'?', GitStatusCode::Untracked},
    {'!', GitStatusCode::Ignored},
    {' ', GitStatusCode::Unmodified},
    {'M', GitStatusCode::Modified},
    {'T', GitStatusCode::FileTypeChanged},
    {'A', GitStatusCode::Added},
    {'D', GitStatusCode::Deleted},
    {'R', GitStatusCode::Renamed},
    {'C', GitStatusCode::Copied},
    {'U', GitStatusCode::UpdatedButUnmerged}
};

QHash<QUrl, QHash<int, QVariantHash> > g_breakpoints = {};
PortModel *g_portModel = {};
QStandardItemModel *g_watchStandardItemModel = {};

#include "globals.h"

GlobalManager *g_globalManager = nullptr;

bool g_terminating = false;
QUrl g_workspaceUrl = {};
QString g_gitRootPath = {};
QJsonObject g_mainConfig = {};
QJsonObject g_workspaceConfig = {};

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

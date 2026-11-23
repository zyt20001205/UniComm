#include "globals.h"

GlobalManager *g_globalManager = nullptr;

bool g_terminating = false;
QUrl g_workspaceUrl = {};
QString g_gitRootPath = {};
QJsonObject g_mainConfig = {};
QJsonObject g_workspaceConfig = {};

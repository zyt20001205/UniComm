#ifndef UNICOMM_GLOBALS_H
#define UNICOMM_GLOBALS_H

#include "mainWindow/mainWindow.h"

extern GlobalManager *g_globalManager;

extern bool g_terminating;
extern QUrl g_workspaceUrl;
extern QJsonObject g_mainConfig;
extern QJsonObject g_workspaceConfig;

namespace Theme {
    enum {
        Light,
        Dark,
    };
}

namespace ToastLevel {
    enum {
        Error,
        Warning,
        Info,
        Success
    };
}

#endif //UNICOMM_GLOBALS_H

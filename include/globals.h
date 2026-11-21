#ifndef UNICOMM_GLOBALS_H
#define UNICOMM_GLOBALS_H

#include "mainWindow/mainWindow.h"

extern GlobalManager *g_globalManager;

extern bool g_terminating;
extern QUrl g_workspaceUrl;
extern QString g_gitRootPath;
extern QJsonObject g_mainConfig;
extern QJsonObject g_workspaceConfig;

extern QVariantMap g_cursorPosition;
extern QHash<QChar, int> g_gitStatusCode;

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

namespace GitStatus {
    enum {
        Idle,
        Transfer,
        Merge,
        Rebase,
        CherryPick
    };
}

namespace GitStatusCode {
    enum {
        /* '?' */Untracked,
        /* '!' */Ignored,
        /* ' ' */Unmodified,
        /* 'M' */Modified,
        /* 'T' */FileTypeChanged,
        /* 'A' */Added,
        /* 'D' */Deleted,
        /* 'R' */Renamed,
        /* 'C' */Copied,
        /* 'U' */UpdatedButUnmerged
    };
}

#endif //UNICOMM_GLOBALS_H

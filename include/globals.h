#ifndef UNICOMM_GLOBALS_H
#define UNICOMM_GLOBALS_H

#include <visa.h>

#include "mainWindow/mainWindow.h"

extern GlobalManager *g_globalManager;

extern bool g_terminating;
extern QUrl g_workspaceUrl;
extern QString g_gitRootPath;
extern QJsonObject g_mainConfig;
extern QJsonObject g_workspaceConfig;

extern ViSession g_rm;

extern QVariantMap g_cursorPosition;
extern QHash<QChar, int> g_gitStatusCode;

namespace Theme {
    enum {
        Light,
        Dark,
    };
}

namespace LogLevel {
    enum {
        Error,
        Warning,
        Info,
        Transmit,
        Receive
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

namespace PortType {
    enum {
        SerialPort,
        Vision,
        TcpClient,
        TcpServer,
        SslClient,
        SslServer,
        WebSocketClient,
        WebSocketServer,
        UdpSocket,
        BluetoothLe,
        Visa
    };
}

namespace WebSocketMessageType {
    enum {
        Text = 1,
        Binary = 2
    };
}

namespace ImagePipeline {
    enum {
        Scale,
        Threshold
    };
}

namespace Recognition {
    enum {
        OCR,
        CornerShiTomasi,
        CornerHarris,
        TemplateMatch
    };
}

namespace InterpreterMode {
    enum {
        Run,
        Debug,
        Agent,
        Terminate
    };
}

namespace Debug {
    enum {
        Terminate,
        Resume,
        Pause,
        StepOver,
        StepInto,
        StepOut,
        RunToCursor
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

struct TerminalCell {
    int width{};
    QString text{};
    bool bold{};
    bool underline{};
    bool italic{};
    bool strike{};
    QColor foreground{};
    QColor background{};
    int uri{};
    bool dim{};
    bool overline{};
};

struct TerminalLine {
    QList<TerminalCell> cells{};
    bool continuation{};
};

struct TerminalSelection {
    qint64 anchorRow{-1};
    int anchorCol{};
    qint64 extentRow{-1};
    int extentCol{};
};

#endif //UNICOMM_GLOBALS_H

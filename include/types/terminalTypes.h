#ifndef UNICOMM_TERMINALTYPES_H
#define UNICOMM_TERMINALTYPES_H

#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace TerminalBackend {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Process,
        Lua
    };

    Q_ENUM_NS(Value)
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

#endif //UNICOMM_TERMINALTYPES_H

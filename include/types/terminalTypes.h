#ifndef UNICOMM_TERMINALTYPES_H
#define UNICOMM_TERMINALTYPES_H

#include <QMetaObject>
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

#endif //UNICOMM_TERMINALTYPES_H

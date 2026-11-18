#ifndef UNICOMM_DEBUGTYPES_H
#define UNICOMM_DEBUGTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace Debug {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Terminate,
        Resume,
        Pause,
        StepOver,
        StepInto,
        StepOut,
        RunToCursor
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_DEBUGTYPES_H

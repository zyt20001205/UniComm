#ifndef UNICOMM_RUNTIMETYPES_H
#define UNICOMM_RUNTIMETYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace RunMode {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Run,
        Debug,
        Agent
    };

    Q_ENUM_NS(Value)
}

namespace InterpreterState {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Resume,
        Pause,
        StepOver,
        StepInto,
        StepOut,
        RunToCursor
    };

    Q_ENUM_NS(Value)
}

namespace ThreadStatus {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Running,
        Paused,
        Terminating
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_RUNTIMETYPES_H

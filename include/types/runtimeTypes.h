#ifndef UNICOMM_RUNTIMETYPES_H
#define UNICOMM_RUNTIMETYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <lua.hpp>

namespace RunMode {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Run,
        Debug
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

namespace LuaType {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        None = LUA_TNONE,
        Nil = LUA_TNIL,
        Boolean = LUA_TBOOLEAN,
        LightUserdata = LUA_TLIGHTUSERDATA,
        Number = LUA_TNUMBER,
        String = LUA_TSTRING,
        Table = LUA_TTABLE,
        Function = LUA_TFUNCTION,
        Userdata = LUA_TUSERDATA,
        Thread = LUA_TTHREAD
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

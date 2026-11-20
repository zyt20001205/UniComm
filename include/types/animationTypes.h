#ifndef UNICOMM_ANIMATIONTYPES_H
#define UNICOMM_ANIMATIONTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace AnimationDuration {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Fade = 83,
        FlashHold = 350,
        Follow = 83,
        Move = 167,
        PaneClose = 100,
        PaneOpen = 200,
        Reorder = 200
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_ANIMATIONTYPES_H

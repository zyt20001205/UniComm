#ifndef UNICOMM_DIFFTYPES_H
#define UNICOMM_DIFFTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace DiffKind {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Added,
        Deleted,
        Modified,
        Conflict
    };

    Q_ENUM_NS(Value)
}

#endif // UNICOMM_DIFFTYPES_H

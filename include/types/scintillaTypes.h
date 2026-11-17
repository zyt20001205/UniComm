#ifndef UNICOMM_SCINTILLATYPES_H
#define UNICOMM_SCINTILLATYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace ScintillaIndicator {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Typo,
        Hint,
        Info,
        Warning,
        Error,
        Password,
        Highlight,
        Read,
        Write,
        Hyperlink,
        Result,
        Current,
        ConflictStart,
        ConflictSeparator,
        ConflictEnd
    };

    Q_ENUM_NS(Value)
}

namespace ScintillaMarker {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        BreakpointEnabled,
        BreakpointDisabled,
        Navigation,
        Debug,
        Hint,
        ConflictCurrent,
        ConflictIncoming
    };

    Q_ENUM_NS(Value)
}

namespace ScintillaStyle {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Unused,
        Namespace,
        Class,
        Type,
        Struct,
        TypeParameter,
        Parameter,
        Variable,
        Property,
        EnumMember,
        Event,
        FunctionCall,
        FunctionDeclaration,
        Method,
        Macro,
        Keyword,
        Comment,
        String,
        Number,
        Operator,
        Decorator
    };

    Q_ENUM_NS(Value)
}

namespace CustomStyle {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Default = 32,
        LineNumber,
        BraceLight,
        BraceBad,
        ControlChar,
        IndentGuide,
        CallTip,
        FoldDisplayText,
        LastPredefined,
        Annotation = 40
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_SCINTILLATYPES_H

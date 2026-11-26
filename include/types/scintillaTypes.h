#ifndef UNICOMM_SCINTILLATYPES_H
#define UNICOMM_SCINTILLATYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace ScintillaCodePage {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        None = -1,
        SingleByte = 0,
        Shift_JIS = 932,
        GBK = 936,
        UHC = 949,
        Big5 = 950,
        Johab = 1361,
        UTF_8 = 65001
    };

    Q_ENUM_NS(Value)
}

namespace ScintillaEolMode {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        None = -1,
        CRLF = 0,
        CR = 1,
        LF = 2
    };

    Q_ENUM_NS(Value)
}

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
        Current
    };

    Q_ENUM_NS(Value)
}

namespace ScintillaMarker {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        BreakpointEnabled,
        BreakpointDisabled,
        Debug,
        DebugLine,
        Hint,
        // Scintilla reserves marker slots 25–31 for folding.
        FolderEnd = 25,
        FolderOpenMid = 26,
        FolderMidTail = 27,
        FolderTail = 28,
        FolderSub = 29,
        Folder = 30,
        FolderOpen = 31
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
        Annotation,
        EolAnnotation
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_SCINTILLATYPES_H

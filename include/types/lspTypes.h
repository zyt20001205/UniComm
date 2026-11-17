#ifndef UNICOMM_LSPTYPES_H
#define UNICOMM_LSPTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace LspCompletionKind {
    Q_NAMESPACE

    QML_ELEMENT

    enum Kind {
        Text = 1,
        Method,
        Function,
        Constructor,
        Field,
        Variable,
        Class,
        Interface,
        Module,
        Property,
        Unit,
        Value,
        Enum,
        Keyword,
        Snippet,
        Color,
        File,
        Reference,
        Folder,
        EnumMember,
        Constant,
        Struct,
        Event,
        Operator,
        TypeParameter
    };

    Q_ENUM_NS(Kind)
}

namespace LspSymbolKind {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        File = 1,
        Module,
        Namespace,
        Package,
        Class,
        Method,
        Property,
        Field,
        Constructor,
        Enum,
        Interface,
        Function,
        Variable,
        Constant,
        String,
        Number,
        Boolean,
        Array,
        Object,
        Key,
        Null,
        EnumMember,
        Struct,
        Event,
        Operator,
        TypeParameter
    };

    Q_ENUM_NS(Value)
}

namespace LspTokenModifiers {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Declaration = 1 << 0,
        Definition = 1 << 1,
        Readonly = 1 << 2,
        Static = 1 << 3,
        Deprecated = 1 << 4,
        Abstract = 1 << 5,
        Async = 1 << 6,
        Modification = 1 << 7,
        Documentation = 1 << 8,
        DefaultLibrary = 1 << 9,
        Global = 1 << 10,
    };

    Q_ENUM_NS(Value)
}

namespace LspTokenType {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Namespace,
        Type,
        Class,
        Enum,
        Interface,
        Struct,
        TypeParameter,
        Parameter,
        Variable,
        Property,
        EnumMember,
        Event,
        Function,
        Method,
        Macro,
        Keyword,
        Modifier,
        Comment,
        String,
        Number,
        RegExp,
        Operator,
        Decorator
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_LSPTYPES_H

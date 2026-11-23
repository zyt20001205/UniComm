#ifndef UNICOMM_GITTYPES_H
#define UNICOMM_GITTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace GitFileStatus {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Untracked,
        Ignored,
        Unmodified,
        Modified,
        FileTypeChanged,
        Added,
        Deleted,
        Renamed,
        Copied,
        Unmerged
    };

    Q_ENUM_NS(Value)

    inline Value fromCode(const QChar code) {
        switch (code.unicode()) {
            case '!': return Ignored;
            case ' ': return Unmodified;
            case 'M': return Modified;
            case 'T': return FileTypeChanged;
            case 'A': return Added;
            case 'D': return Deleted;
            case 'R': return Renamed;
            case 'C': return Copied;
            case 'U': return Unmerged;
            case '?':
            default: return Untracked;
        }
    }
}

namespace GitOperationState {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Idle,
        Transfer,
        Merge,
        Rebase,
        CherryPick
    };

    Q_ENUM_NS(Value)
}

namespace GitResetMode {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Mixed,
        Soft,
        Hard,
        Merge,
        Keep
    };

    Q_ENUM_NS(Value)
}

namespace GitRestoreMode {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value {
        Worktree,
        Staged,
        Both
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_GITTYPES_H

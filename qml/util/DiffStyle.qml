pragma Singleton

import QtQml

import UniComm

QtObject {
    function diffColor(kind: int): var {
        const colors = global.theme.general
        switch (kind) {
            case DiffKind.Added:
                return { back: colors.seafoamBack2, fore: colors.seafoamFore2 }
            case DiffKind.Deleted:
                return { back: colors.platinumBack2, fore: colors.platinumFore2 }
            case DiffKind.Modified:
                return { back: colors.blueBack2, fore: colors.blueFore2 }
            case DiffKind.Conflict:
                return { back: colors.brownBack2, fore: colors.brownFore2 }
        }
    }
}

pragma Singleton

import QtQml

import UniComm

QtObject {
    function color(status): string {
        const colors = global.theme.general
        switch (status) {
            case GitFileStatus.Untracked:
                return colors.successFore3
            case GitFileStatus.Ignored:
            case GitFileStatus.Deleted:
                return colors.stroke
            case GitFileStatus.Modified:
                return colors.brandBack
            case GitFileStatus.Added:
                return colors.successFore2
            case GitFileStatus.Renamed:
                return colors.warningFore3
            case GitFileStatus.Unmerged:
                return colors.dangerFore3
            default:
                return colors.fore
        }
    }
}

pragma Singleton

import QtQml

import UniComm

QtObject {
    function refIcon(type: int): string {
        switch (type) {
            case GitRefType.LocalGroup:
                return "qrc:/icon/tcpClient.svg"
            case GitRefType.RemoteGroup:
                return "qrc:/icon/tcpServer.svg"
            case GitRefType.Tag:
            case GitRefType.TagGroup:
            case GitRefType.Current:
                return "qrc:/icon/tag.svg"
            case GitRefType.Upstream:
                return "qrc:/icon/link.svg"
            default:
                return "qrc:/icon/gitBranch.svg"
        }
    }

    function refColor(type: int): string {
        const colors = global.theme.general
        return type === GitRefType.Current || type === GitRefType.Upstream ? colors.warningFore3 : colors.fore
    }

    function fileStatusColor(status): string {
        const colors = global.theme.general
        switch (status) {
            case GitFileStatus.Untracked:
                return colors.cranberryFore2
            case GitFileStatus.Ignored:
                return colors.pumpkinFore2
            case GitFileStatus.Deleted:
                return colors.foreDisabled
            case GitFileStatus.Modified:
                return colors.royalBlueFore2
            case GitFileStatus.FileTypeChanged:
                return colors.berryFore2
            case GitFileStatus.Added:
                return colors.greenFore2
            case GitFileStatus.Renamed:
                return colors.lavenderFore2
            case GitFileStatus.Copied:
                return colors.tealFore2
            case GitFileStatus.Unmerged:
                return colors.redFore2
            default:
                return colors.fore
        }
    }
}

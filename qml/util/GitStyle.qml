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

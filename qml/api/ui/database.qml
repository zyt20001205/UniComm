pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQml.Models

Item {
    id: rootItem

    required property AbstractItemModel dataModel

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    TableView {
        id: tableView
        anchors.fill: parent
        alternatingRows: false
        clip: true
        rowSpacing: 1; columnSpacing: 1
        resizableColumns: true
        editTriggers: TableView.NoEditTriggers
        model: rootItem.dataModel
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            policy: ScrollBar.AsNeeded
            palette {
                mid: global.stroke
                dark: global.strokePressed
            }
        }

        Rectangle {
            anchors.fill: parent
            color: global.stroke
        }

        delegate: DatabaseCell {}
    }

    component DatabaseCell: Item {
        id: cell
        implicitWidth: {
            if (column === 0) return Math.max(60, Math.min(textMetrics.width + 12, tableView.width / 2))
            return Math.max(60, tableView.contentWidth - tableView.ScrollBar.vertical.width - Math.max(0, tableView.columnWidth(0)) - tableView.columnSpacing)
        }
        implicitHeight: 32
        required property int column
        required property string display

        function flash() {
            flashAnimation.restart()
        }

        TableView.onPooled: {
            flashAnimation.stop()
            highlight.opacity = 0
        }

        Rectangle {
            anchors.fill: parent
            color: global.back
        }

        Rectangle {
            id: highlight
            anchors.fill: parent
            radius: 6
            color: global.backHover
            opacity: 0
        }

        SequentialAnimation {
            id: flashAnimation
            NumberAnimation { target: highlight; property: "opacity"; to: 1; duration: 150 }
            PauseAnimation { duration: 350 }
            NumberAnimation { target: highlight; property: "opacity"; to: 0; duration: 150 }
        }

        TextMetrics {
            id: textMetrics
            font: label.font
            text: cell.display
        }

        Label {
            id: label
            anchors.fill: parent
            leftPadding: 6
            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
            text: cell.display
            elide: Text.ElideRight
        }
    }

    Connections {
        target: rootItem.dataModel

        function onDataChanged(topLeft, bottomRight, roles) {
            if (topLeft.column !== 1) return
            const cell = tableView.itemAtIndex(topLeft) as DatabaseCell
            if (cell) cell.flash()
        }
    }
}

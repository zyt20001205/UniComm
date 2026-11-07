import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: rootItem

    required property var dataModel

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: exportButton
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                text: qsTr("Export CSV")
                display: AbstractButton.IconOnly
                icon.source: "qrc:/icon/share.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                ToolTip.text: exportButton.text
                ToolTip.visible: hovered
                ToolTip.delay: 400

                onClicked: exportDialog.open()
            }
        }

        Item {
            id: tableArea
            Layout.fillWidth: true; Layout.fillHeight: true

            HorizontalHeaderView {
                id: headerView
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: tableView.right
                height: 32
                syncView: tableView
                clip: true
                interactive: false
                resizableColumns: true
                editTriggers: TableView.NoEditTriggers

                delegate: TableHeader {}
            }

            TableView {
                id: tableView
                anchors.top: headerView.bottom; anchors.left: parent.left
                anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.rightMargin: verticalScrollBar.width
                anchors.bottomMargin: horizontalScrollBar.height
                alternatingRows: false
                clip: true
                rowSpacing: 1; columnSpacing: 1
                boundsBehavior: Flickable.StopAtBounds
                resizableColumns: true
                editTriggers: TableView.NoEditTriggers
                model: rootItem.dataModel

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: tableArea
                    anchors.left: tableView.right; anchors.top: tableView.top; anchors.bottom: tableView.bottom
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    id: horizontalScrollBar
                    parent: tableArea
                    anchors.left: tableView.left; anchors.right: tableView.right; anchors.top: tableView.bottom
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
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

                delegate: Rectangle {
                    id: cell
                    implicitWidth: 120; implicitHeight: 32
                    required property var display
                    color: global.back

                    Label {
                        anchors.fill: parent
                        leftPadding: 6; rightPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: cell.display ?? ""
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    component TableHeader: Rectangle {
        id: headerCell
        implicitWidth: 120; implicitHeight: 32
        required property string display
        color: global.backHover

        function flash() {
            flashAnimation.restart()
        }

        TableView.onPooled: {
            flashAnimation.stop()
            highlight.opacity = 0
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

        Label {
            anchors.fill: parent
            leftPadding: 6; rightPadding: 6
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            text: headerCell.display
            elide: Text.ElideRight
        }
    }

    Connections {
        target: rootItem.dataModel

        function onDataChanged(topLeft, bottomRight, roles) {
            for (let column = topLeft.column; column <= bottomRight.column; ++column) {
                const header = headerView.itemAtCell(Qt.point(column, 0)) as TableHeader
                if (header) header.flash()
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export CSV")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("CSV files (*.csv)")]
        defaultSuffix: "csv"

        onAccepted: {
            const exception = rootItem.dataModel._export(selectedFile)
            if (exception !== "") {
                exportErrorDialog.text = exception
                exportErrorDialog.open()
            }
        }
    }

    MessageDialog {
        id: exportErrorDialog
        title: qsTr("Export failed")
        buttons: MessageDialog.Ok
    }
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent
    readonly property var selectedDiagnostic: diagnosticsModule.diagnostics[tabBar.currentIndex] ?? null

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    Item {
        anchors.fill: parent
        visible: diagnosticsModule.diagnostics.length === 0

        RowLayout {
            anchors.centerIn: parent

            Label {
                text: qsTr("No errors found.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/checkmarkCircle.svg"
                color: global.successBack3
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: diagnosticsModule.diagnostics.length !== 0

        TabBar {
            id: tabBar
            contentHeight: 24
            Layout.fillWidth: true; Layout.preferredHeight: 32
            currentIndex: {
                const diagnostics = diagnosticsModule.diagnostics
                const focusedUrl = String(global.documentModule.focusedUrl)
                // Wait for the Repeater to finish rebuilding its tabs.
                if (count !== diagnostics.length)
                    return -1
                for (let i = 0; i < diagnostics.length; ++i) {
                    if (String(diagnostics[i].documentUrl) === focusedUrl)
                        return i
                }
                return diagnostics.length ? 0 : -1
            }

            Repeater {
                model: diagnosticsModule.diagnostics

                delegate: TabButton {
                    required property var modelData
                    text: modelData.name
                    width: contentItem.implicitWidth + 24; height: 24
                }
            }
        }

        Item {
            id: tablePane
            Layout.fillWidth: true; Layout.fillHeight: true

            HorizontalHeaderView {
                id: horizontalHeaderView
                anchors.left: tableView.left; anchors.right: tableView.right; anchors.top: parent.top
                height: 32
                syncView: tableView
                clip: true
                interactive: false
                resizableColumns: true
                columnSpacing: 0
                editTriggers: TableView.NoEditTriggers

                delegate: Item {
                    id: headerCell
                    required property string display
                    implicitWidth: 120; implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        color: global.back
                    }

                    Label {
                        anchors.fill: parent
                        text: headerCell.display
                        leftPadding: 6; rightPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }

            TableView {
                id: tableView
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: horizontalHeaderView.bottom; anchors.bottom: parent.bottom
                anchors.rightMargin: verticalScrollBar.width
                anchors.bottomMargin: horizontalScrollBar.height
                model: rootItem.selectedDiagnostic ? rootItem.selectedDiagnostic.model : null
                rowSpacing: 1; columnSpacing: 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                editTriggers: TableView.NoEditTriggers

                readonly property int iconWidth: 24
                readonly property int sourceWidth: 120
                readonly property int codeWidth: 100
                readonly property int dataWidth: 120

                columnWidthProvider: function(column: int): real {
                    const resizedWidth = explicitColumnWidth(column)
                    if (resizedWidth >= 0) return resizedWidth

                    const defaultWidths = [iconWidth, sourceWidth, codeWidth, dataWidth]
                    if (column < defaultWidths.length) return defaultWidths[column]

                    let usedWidth = 0
                    for (let i = 0; i < defaultWidths.length; ++i) {
                        const explicitWidth = explicitColumnWidth(i)
                        usedWidth += explicitWidth >= 0 ? explicitWidth : defaultWidths[i]
                    }
                    return Math.max(160, Math.floor(width) - usedWidth)
                }

                onWidthChanged: forceLayout()
                onModelChanged: {
                    contentX = 0
                    contentY = 0
                }

                Rectangle {
                    parent: tableView.contentItem
                    anchors.fill: parent
                    color: global.stroke
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: tablePane
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
                    parent: tablePane
                    anchors.left: tableView.left; anchors.right: tableView.right; anchors.top: tableView.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                delegate: Item {
                    id: cell
                    required property int column
                    required property var display
                    required property var decoration
                    required property int severity
                    required property var position
                    required property string message
                    implicitWidth: 120; implicitHeight: 24

                    Rectangle {
                        anchors.fill: parent
                        color: global.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: global.backHover
                        opacity: hoverHandler.hovered ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        visible: cell.column === 0
                        source: cell.decoration ?? ""
                        color: cell.severity === 1 ? global.dangerBack3
                               : cell.severity === 2 ? global.warningBack3
                               : cell.severity === 3 ? global.brandBack
                               : global.fore
                    }

                    Label {
                        anchors.fill: parent
                        visible: cell.column !== 0
                        text: String(cell.display ?? "")
                        textFormat: cell.column === 4 ? Text.RichText : Text.PlainText
                        leftPadding: 6; rightPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        id: hoverHandler
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: diagnosticsModule.indicatorFill(cell.position)
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: {
                            diagnosticMenu.position = cell.position
                            diagnosticMenu.diagnostic = cell.message
                            diagnosticMenu.popup()
                        }
                    }
                }
            }
        }
    }
}

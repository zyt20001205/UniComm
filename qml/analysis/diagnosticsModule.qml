pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    id: root
    anchors.fill: parent
    required property DiagnosticsModule diagnosticsModule
    readonly property var selectedDiagnostic: root.diagnosticsModule.diagnostics[tabBar.currentIndex] ?? null

    Rectangle {
        anchors.fill: parent
        color: global.theme.back
    }

    Item {
        anchors.fill: parent
        visible: root.diagnosticsModule.diagnostics.length === 0

        RowLayout {
            anchors.centerIn: parent

            Label {
                text: qsTr("No errors found.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/checkmarkCircle.svg"
                color: global.theme.successBack3
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: root.diagnosticsModule.diagnostics.length !== 0

        TabBar {
            id: tabBar
            contentHeight: 24
            Layout.fillWidth: true; Layout.preferredHeight: 32
            currentIndex: {
                const diagnostics = root.diagnosticsModule.diagnostics
                const focusedUrl = global.documentModule.focusedUrl
                // Wait for the Repeater to finish rebuilding its tabs.
                if (count !== diagnostics.length)
                    return -1
                for (let i = 0; i < diagnostics.length; ++i) {
                    if (diagnostics[i].documentUrl === focusedUrl)
                        return i
                }
                return diagnostics.length ? 0 : -1
            }

            Repeater {
                model: root.diagnosticsModule.diagnostics

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
                        color: global.theme.back
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
                model: root.selectedDiagnostic ? root.selectedDiagnostic.model : null
                rowSpacing: 1; columnSpacing: 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                editTriggers: TableView.NoEditTriggers

                readonly property var defaultColumnWidths: ({
                    [DiagnosticsModule.Col.Severity]: 24,
                    [DiagnosticsModule.Col.Source]: 120,
                    [DiagnosticsModule.Col.Code]: 100,
                    [DiagnosticsModule.Col.Data]: 120
                })

                columnWidthProvider: function (column: int): real {
                    if (column !== DiagnosticsModule.Col.Message) {
                        const explicitWidth = explicitColumnWidth(column)
                        return explicitWidth >= 0 ? explicitWidth : defaultColumnWidths[column]
                    }

                    let remainingWidth = Math.floor(width)
                    for (let i = DiagnosticsModule.Col.Severity; i < DiagnosticsModule.Col.Message; ++i) {
                        const explicitWidth = explicitColumnWidth(i)
                        remainingWidth -= explicitWidth >= 0 ? explicitWidth : defaultColumnWidths[i]
                    }
                    return Math.max(160, remainingWidth)
                }

                onWidthChanged: forceLayout()
                onModelChanged: {
                    contentX = 0
                    contentY = 0
                }

                Rectangle {
                    parent: tableView.contentItem
                    anchors.fill: parent
                    color: global.theme.stroke
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: tablePane
                    anchors.left: tableView.right; anchors.top: tableView.top; anchors.bottom: tableView.bottom
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.stroke
                        dark: global.theme.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    id: horizontalScrollBar
                    parent: tablePane
                    anchors.left: tableView.left; anchors.right: tableView.right; anchors.top: tableView.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.stroke
                        dark: global.theme.strokePressed
                    }
                }

                delegate: Item {
                    id: cell
                    required property int column
                    required property var display
                    required property int severity
                    required property url documentUrl
                    required property int startLine
                    required property int startCharacter
                    required property int endLine
                    required property int endCharacter
                    required property string message
                    implicitWidth: 120; implicitHeight: 24

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: global.theme.backHover
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
                        visible: cell.column === DiagnosticsModule.Col.Severity
                        source: {
                            switch (cell.severity) {
                                case DiagnosticsModule.Level.Error:
                                    return "qrc:/icon/error.svg"
                                case DiagnosticsModule.Level.Warning:
                                    return "qrc:/icon/warning.svg"
                                case DiagnosticsModule.Level.Information:
                                    return "qrc:/icon/info.svg"
                                case DiagnosticsModule.Level.Hint:
                                    return "qrc:/icon/hint.svg"
                                default:
                                    return ""
                            }
                        }
                        color: {
                            switch (cell.severity) {
                                case DiagnosticsModule.Level.Error:
                                    return global.theme.dangerBack3
                                case DiagnosticsModule.Level.Warning:
                                    return global.theme.warningBack3
                                case DiagnosticsModule.Level.Information:
                                    return global.theme.brandBack
                                default:
                                    return global.theme.fore
                            }
                        }
                    }

                    Label {
                        anchors.fill: parent
                        visible: cell.column !== DiagnosticsModule.Col.Severity
                        text: String(cell.display ?? "")
                        textFormat: cell.column === DiagnosticsModule.Col.Message ? Text.RichText : Text.PlainText
                        leftPadding: 6; rightPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        id: hoverHandler
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: {
                            root.diagnosticsModule.setIndex(cell.documentUrl, cell.startLine, cell.startCharacter)
                            root.diagnosticsModule.fillIndicator(cell.documentUrl, 6, cell.startLine, cell.startCharacter, cell.endLine, cell.endCharacter, 1000)
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: {
                            diagnosticMenu.documentUrl = cell.documentUrl
                            diagnosticMenu.startLine = cell.startLine
                            diagnosticMenu.startCharacter = cell.startCharacter
                            diagnosticMenu.endLine = cell.endLine
                            diagnosticMenu.endCharacter = cell.endCharacter
                            diagnosticMenu.diagnostic = cell.message
                            diagnosticMenu.popup()
                        }
                    }
                }
            }
        }
    }

    Menu {
        id: diagnosticMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string diagnostic
        property url documentUrl
        property int startLine
        property int startCharacter
        property int endLine
        property int endCharacter

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.copyToClipboard(diagnosticMenu.diagnostic)
        }

        MenuItem {
            text: qsTr("View")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                root.diagnosticsModule.setIndex(diagnosticMenu.documentUrl, diagnosticMenu.startLine, diagnosticMenu.startCharacter)
                root.diagnosticsModule.fillIndicator(diagnosticMenu.documentUrl, 6, diagnosticMenu.startLine, diagnosticMenu.startCharacter, diagnosticMenu.endLine, diagnosticMenu.endCharacter, 1000)
            }
        }
    }
}

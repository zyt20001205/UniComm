pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import UniComm

Item {
    id: root
    required property LogModule logModule
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ColumnLayout {
            Layout.preferredWidth: 24; Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.margins: 4

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: "qrc:/icon/clock.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                checked: root.logModule.timestamp
                onToggled: root.logModule.timestamp = checked

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = parent.checked ? qsTr("Hide Timestamp") : qsTr("Show Timestamp")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: "qrc:/icon/wrap.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                checked: root.logModule.wrap
                onToggled: root.logModule.wrap = checked

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = parent.checked ? qsTr("Disable Wrap") : qsTr("Enable Wrap")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/save.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: fileDialog.open()

                FileDialog {
                    id: fileDialog
                    currentFolder: StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0]
                    fileMode: FileDialog.SaveFile
                    nameFilters: ["Plain Text (*.txt)", "PDF (*.pdf)", "Rich Text (*.html)"]
                    defaultSuffix: selectedNameFilter.extensions[0]
                    selectedFile: currentFolder + "/log_" + Qt.formatDateTime(new Date(), "yyyyMMdd_HHmmss")
                    onAccepted: root.logModule.logSave(selectedFile)
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Save")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onToggled: {
                    if (!checked) {
                        textArea.clear()
                    }
                }

                Timer {
                    interval: 1000
                    running: parent.checked
                    onTriggered: parent.checked = false
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = parent.checked ? qsTr("Confirm") : qsTr("Clear")
                    }
                }
            }
        }

        ScrollView {
            rightPadding: effectiveScrollBarWidth
            bottomPadding: effectiveScrollBarHeight
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.topMargin: 4; Layout.rightMargin: 4; Layout.bottomMargin: 4

            ScrollBar.vertical.minimumSize: Math.min(1, 20 / Math.max(1, ScrollBar.vertical.height))
            ScrollBar.vertical.palette.mid: global.theme.general.stroke
            ScrollBar.vertical.palette.dark: global.theme.general.strokePressed
            ScrollBar.horizontal.minimumSize: Math.min(1, 20 / Math.max(1, ScrollBar.horizontal.width))
            ScrollBar.horizontal.palette.mid: global.theme.general.stroke
            ScrollBar.horizontal.palette.dark: global.theme.general.strokePressed

            TextArea {
                id: textArea
                readOnly: true
                textFormat: TextEdit.RichText
                verticalAlignment: TextEdit.AlignTop
                wrapMode: root.logModule.wrap ? TextEdit.Wrap : TextEdit.NoWrap
                font: root.logModule.logFont
                ContextMenu.menu: null
                Component.onCompleted: root.logModule.documentSet(textDocument)

                Connections {
                    target: root.logModule
                    function onAppendText(text) { textArea.append(text) }
                }

                HoverHandler {
                    id: hoverHandler
                    cursorShape: textArea.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                }

                ToolTip {
                    id: tooltip
                    visible: textArea.hoveredLink
                    text: qsTr("Ctrl + Click")
                    x: hoverHandler.point.position.x + 10
                    y: hoverHandler.point.position.y + 10
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    acceptedModifiers: Qt.ControlModifier
                    onTapped: {
                        if (textArea.hoveredLink) {
                            const url = textArea.hoveredLink
                            if (url.startsWith("request.expand://")) {
                                mainTextView.position = textArea.mapToGlobal(point.position)
                                mainTextView.font = textArea.font
                                mainTextView.data = root.logModule.linkTextGet(url)
                                mainTextView.open()
                            } else {
                                Qt.openUrlExternally(url)
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        if (textArea.hoveredLink && !textArea.hoveredLink.startsWith("request.expand://")) {
                            mainLinkMenu.url = textArea.hoveredLink
                            mainLinkMenu.popup()
                        } else {
                            const position = mainOverlay.mapFromGlobal(textArea.mapToGlobal(point.position))
                            contextMenu.popup(position.x, position.y)
                        }
                    }
                }
            }
        }
    }

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        Menu {
            title: qsTr("Fold at")
            icon.source: "qrc:/icon/arrowAutoFitWidth.svg"
            icon.width: 16; icon.height: 16
            ButtonGroup { id: collapseThresholdGroup }
            Repeater {
                model: [100, 200, 500, 1000]
                delegate: MenuItem {
                    required property int modelData
                    text: qsTr("%1 characters").arg(modelData)
                    checkable: true
                    checked: root.logModule.collapseThreshold === modelData
                    ButtonGroup.group: collapseThresholdGroup
                    onTriggered: root.logModule.collapseThreshold = modelData
                }
            }
        }

        Menu {
            title: qsTr("Keep last")
            icon.source: "qrc:/icon/arrowAutoFitHeight.svg"
            icon.width: 16; icon.height: 16
            ButtonGroup { id: lineLimitGroup }
            Repeater {
                model: [1000, 2000, 5000, 10000]
                delegate: MenuItem {
                    required property int modelData
                    text: qsTr("%1 lines").arg(modelData)
                    checkable: true
                    checked: root.logModule.lineLimit === modelData
                    ButtonGroup.group: lineLimitGroup
                    onTriggered: root.logModule.lineLimit = modelData
                }
            }
        }
    }
}

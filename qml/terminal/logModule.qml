import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ColumnLayout {
            Layout.preferredWidth: 24; Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.margins: 4

            Button {
                id: timestampButton
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: "qrc:/icon/clock.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onToggled: logModule.timestampToggle(checked)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = parent.checked ? qsTr("Hide Timestamp") : qsTr("Show Timestamp")
                    }
                }
            }

            Button {
                id: wrapButton
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: "qrc:/icon/wrap.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onToggled: logModule.wrapToggle(checked)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = parent.checked ? qsTr("Disable Wrap") : qsTr("Enable Wrap")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/autoFitHeight.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: heightDialog.open()

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Maximum Line Count")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/save.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    if (logModule.logSaveCheck()) {
                        fileDialog.open()
                    }
                }

                FileDialog {
                    id: fileDialog
                    currentFolder: StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0]
                    fileMode: FileDialog.SaveFile
                    nameFilters: ["Plain Text (*.txt)", "PDF (*.pdf)", "Rich Text (*.html)"]
                    selectedFile: currentFolder + "/log_" + Qt.formatDateTime(new Date(), "yyyyMMdd_HHmmss")
                    onAccepted: logModule.logSave(selectedFile)
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Save")
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
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = parent.checked ? qsTr("Confirm") : qsTr("Clear")
                    }
                }
            }
        }

        ScrollView {
            id: scrollView
            rightPadding: effectiveScrollBarWidth
            bottomPadding: effectiveScrollBarHeight
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.topMargin: 4; Layout.rightMargin: 4; Layout.bottomMargin: 4

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                x: parent.mirrored ? 0 : parent.width - width
                y: parent.topPadding
                height: parent.availableHeight
                active: parent.ScrollBar.horizontal.active
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            ScrollBar.horizontal: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, width))
                x: parent.leftPadding
                y: parent.height - height
                width: parent.availableWidth
                active: parent.ScrollBar.vertical.active
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            TextArea {
                id: textArea
                readOnly: true
                textFormat: TextEdit.RichText
                verticalAlignment: TextEdit.AlignTop
                wrapMode: wrapButton.checked ? TextEdit.Wrap : TextEdit.NoWrap
                ContextMenu.menu: null

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
                                logModule.linkClick(url)
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
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            const objects = {
                "timestampButton": timestampButton,
                "wrapButton": wrapButton,
                "textArea": textArea
            };
            logModule.propertyGet(objects)
        }
    }
}

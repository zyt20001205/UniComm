pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    id: root
    required property StatusModule statusModule
    required property string workspaceName
    readonly property var tasks: statusModule.tasks
    height: 24

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    RowLayout {
        anchors.fill: parent

        Button {
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            icon.source: "qrc:/icon/home.svg"
            icon.width: 16; icon.height: 16
            text: root.workspaceName
            Layout.preferredHeight: 20
        }

        RowLayout {
            Repeater {
                model: root.statusModule.documentPathGet(global.documentModule.focusedUrl)

                delegate: RowLayout {
                    id: breadcrumbItem
                    Layout.alignment: Qt.AlignVCenter
                    required property string modelData

                    IconImage {
                        color: global.theme.general.fore
                        source: "qrc:/icon/arrowCollapse.svg"
                        sourceSize.width: 16
                        sourceSize.height: 16
                        Layout.preferredHeight: 16
                    }

                    Button {
                        id: breadcrumbButton
                        flat: true
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        text: breadcrumbItem.modelData
                        Layout.preferredWidth: breadcrumbButtonTextMetrics.width + 8; Layout.preferredHeight: 20

                        TextMetrics {
                            id: breadcrumbButtonTextMetrics
                            text: breadcrumbButton.text
                            font: breadcrumbButton.font
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            visible: root.tasks.length > 0

            Button {
                id: backgroundButton
                text: root.tasks.length === 1 ? root.tasks[0].message : qsTr("%1 tasks running.").arg(root.tasks.length)
                flat: true
                Layout.preferredHeight: 24

                onClicked: {
                    if (root.tasks.length === 1) root.statusModule.backgroundInfo(root.tasks[0].taskId)
                    else backgroundTooltip.openAboveButton()
                }
            }

            ProgressBar {
                indeterminate: true
                Layout.preferredWidth: 160
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: root.tasks.length > 1 ? "qrc:/icon/moreHorizontal.svg" : "qrc:/icon/dismiss.svg"
                icon.width: 12; icon.height: 12
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    if (root.tasks.length === 1) root.statusModule.backgroundAbort(root.tasks[0].taskId)
                    else backgroundTooltip.openAboveButton()
                }
            }
        }

        Button {
            id: positionButton
            flat: true
            text: {
                const selection = global.documentModule.selection
                if (selection.line === undefined) return ""
                let position = (selection.line + 1) + ":" + selection.character
                if (selection.characters) position += qsTr(" [%1 chars]").arg(selection.characters)
                if (selection.lines) position += qsTr(" [%1 lines]").arg(selection.lines)
                return position
            }
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            Layout.preferredWidth: positionButtonTextMetrics.width + 8; Layout.preferredHeight: 20

            enabled: text.length > 0
            onClicked: global.documentModule.documentGoto()

            TextMetrics {
                id: positionButtonTextMetrics
                text: positionButton.text
                font: positionButton.font
            }
        }

        Button {
            id: eolModeButton
            text: global.documentModule.eolMode === ScintillaEolMode.None ? ""
                : Qt.enumValueToString(ScintillaEolMode.Value, global.documentModule.eolMode)
            enabled: text.length > 0
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            Layout.preferredWidth: eolModeButtonTextMetrics.width + 8; Layout.preferredHeight: 20

            onClicked: {
                const position = mainOverlay.mapFromGlobal(eolModeButton.mapToGlobal(0, 0))
                eolModeMenu.popup(position.x, position.y - eolModeMenu.implicitHeight)
            }

            TextMetrics {
                id: eolModeButtonTextMetrics
                text: eolModeButton.text
                font: eolModeButton.font
            }
        }

        Button {
            id: codePageButton
            text: global.documentModule.codePage === ScintillaCodePage.None ? ""
                : Qt.enumValueToString(ScintillaCodePage.Value, global.documentModule.codePage).replace("_", "-")
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            Layout.preferredWidth: codePageButtonTextMetrics.width + 8; Layout.preferredHeight: 20

            TextMetrics {
                id: codePageButtonTextMetrics
                text: codePageButton.text
                font: codePageButton.font
            }
        }
    }

    Menu {
        id: eolModeMenu
        parent: mainOverlay
        popupType: Popup.Item
        readonly property url documentUrl: global.documentModule.focusedUrl
        onDocumentUrlChanged: if (visible) close()

        onAboutToShow: viewEolItem.checked = global.documentModule.eolViewGet(documentUrl)
        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        Repeater {
            model: [ScintillaEolMode.CRLF, ScintillaEolMode.CR, ScintillaEolMode.LF]
            delegate: MenuItem {
                required property int modelData
                text: Qt.enumValueToString(ScintillaEolMode.Value, modelData)
                enabled: global.documentModule.eolMode !== modelData
                onTriggered: global.documentModule.eolModeSet(eolModeMenu.documentUrl, modelData)
            }
        }
        MenuSeparator {
        }
        MenuItem {
            id: viewEolItem
            text: qsTr("View EOL")
            checkable: true
            onTriggered: global.documentModule.eolViewSet(eolModeMenu.documentUrl, checked)
        }
    }

    ToolTip {
        id: backgroundTooltip
        parent: mainOverlay
        popupType: Popup.Item
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        padding: 6

        function openAboveButton() {
            const position = mainOverlay.mapFromGlobal(backgroundButton.mapToGlobal(0, 0))
            x = position.x
            y = position.y - implicitHeight
            open()
        }

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        contentItem: ListView {
            id: taskView
            implicitWidth: 400; implicitHeight: 300
            clip: true
            spacing: 1
            model: root.tasks
            onCountChanged: if (count < 2 && backgroundTooltip.visible) backgroundTooltip.close()

            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: Rectangle {
                id: task
                required property var modelData
                width: taskView.width; height: 48
                color: global.theme.general.back

                ColumnLayout {
                    anchors.fill: parent
                    anchors.rightMargin: verticalScrollBar.width

                    RowLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true

                        Label {
                            leftPadding: 6
                            text: task.modelData.message
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Button {
                            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                            flat: true
                            icon.source: "qrc:/icon/dismiss.svg"
                            icon.width: 16; icon.height: 16
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24
                            onClicked: root.statusModule.backgroundAbort(task.modelData.taskId)
                        }
                    }

                    ProgressBar {
                        indeterminate: true
                        Layout.fillWidth: true; Layout.preferredHeight: 12
                    }
                }
            }
        }
    }
}

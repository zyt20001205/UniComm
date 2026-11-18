pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import UniComm

Rectangle {
    id: root
    required property DebugModule debugModule
    anchors.fill: parent
    color: global.theme.general.back

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        RowLayout {
            enabled: root.debugModule.currentThread.length > 0
            spacing: 0

            component DebugButton: Button {
                id: button
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = button.mapToGlobal(point.position)
                        mainTooltip.text = button.Accessible.name
                    }
                }
            }

            DebugButton {
                icon.source: "qrc:/icon/stop.svg"
                Accessible.name: qsTr("Terminate")
                onClicked: root.debugModule.stateSet(Debug.Terminate)
            }

            DebugButton {
                icon.source: "qrc:/icon/play.svg"
                Accessible.name: qsTr("Resume")
                onClicked: root.debugModule.stateSet(Debug.Resume)
            }

            DebugButton {
                icon.source: "qrc:/icon/pause.svg"
                Accessible.name: qsTr("Pause")
                onClicked: root.debugModule.stateSet(Debug.Pause)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepOver.svg"
                Accessible.name: qsTr("Step Over")
                onClicked: root.debugModule.stateSet(Debug.StepOver)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepInto.svg"
                Accessible.name: qsTr("Step Into")
                onClicked: root.debugModule.stateSet(Debug.StepInto)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepOut.svg"
                Accessible.name: qsTr("Step Out")
                onClicked: root.debugModule.stateSet(Debug.StepOut)
            }
        }

        ComboBox {
            id: threadComboBox
            Layout.fillWidth: true
            model: root.debugModule.threads
            currentValue: root.debugModule.currentThread
            Accessible.name: qsTr("Debug thread")
            onActivated: root.debugModule.currentThread = currentText

            delegate: ItemDelegate {
                required property int index
                required property string modelData
                width: ListView.view.width
                height: 24
                leftPadding: 11; rightPadding: 6; topPadding: 0; bottomPadding: 0
                text: modelData
                highlighted: threadComboBox.highlightedIndex === index
            }

            popup.parent: mainOverlay
            popup.popupType: Popup.Item
            popup.focus: true
            popup.contentItem.focus: true
            popup.contentItem.Keys.forwardTo: [threadComboBox]
            popup.closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            popup.height: Math.min(threadComboBox.count, 8) * 24 + popup.topPadding + popup.bottomPadding
            popup.topMargin: -1
            popup.bottomMargin: -1
            popup.onAboutToShow: {
                const position = threadComboBox.mapToGlobal(0, threadComboBox.height)
                const localPosition = mainOverlay.mapFromGlobal(position)
                popup.x = localPosition.x
                popup.y = localPosition.y
                global.mainWindow.overlayFlagSet(false, true)
                mainOverlay.widgetCount += 1
            }
            popup.onClosed: mainOverlay.widgetCount -= 1
        }

        ListView {
            id: stackView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            model: root.debugModule.callStack
            focusPolicy: Qt.ClickFocus
            highlightMoveDuration: 0

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: ItemDelegate {
                id: frame
                required property int index
                required property var modelData
                width: stackView.width
                height: 24
                leftPadding: 11; rightPadding: 6; topPadding: 0; bottomPadding: 0
                focusPolicy: Qt.ClickFocus
                highlighted: ListView.isCurrentItem

                contentItem: RowLayout {
                    spacing: 6

                    Label {
                        text: frame.modelData.name
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: frame.modelData.line >= 0 ? frame.modelData.line + 1 : ""
                    }

                    Label {
                        text: frame.modelData.functionName
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                onClicked: {
                    stackView.currentIndex = frame.index
                    if (frame.modelData.line >= 0 && frame.modelData.documentUrl != ""){
                        root.debugModule.setIndex(frame.modelData.documentUrl, frame.modelData.line, 0)
                        root.debugModule.addMarker(modelData.documentUrl, ScintillaMarker.Hint, modelData.line, 1000)
                    }
                }
            }
        }
    }
}

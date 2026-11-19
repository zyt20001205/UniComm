pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
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
        enabled: root.debugModule.currentThread.length > 0
        spacing: 4

        RowLayout {
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
                onClicked: global.threadpoolModule.threadStatusSet(root.debugModule.currentThread, ThreadStatus.Terminating)
            }

            DebugButton {
                icon.source: "qrc:/icon/play.svg"
                Accessible.name: qsTr("Resume")
                onClicked: root.debugModule.interpreterStateSet(InterpreterState.Resume)
            }

            DebugButton {
                icon.source: "qrc:/icon/pause.svg"
                Accessible.name: qsTr("Pause")
                onClicked: root.debugModule.interpreterStateSet(InterpreterState.Pause)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepOver.svg"
                Accessible.name: qsTr("Step Over")
                onClicked: root.debugModule.interpreterStateSet(InterpreterState.StepOver)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepInto.svg"
                Accessible.name: qsTr("Step Into")
                onClicked: root.debugModule.interpreterStateSet(InterpreterState.StepInto)
            }

            DebugButton {
                icon.source: "qrc:/icon/debugStepOut.svg"
                Accessible.name: qsTr("Step Out")
                onClicked: root.debugModule.interpreterStateSet(InterpreterState.StepOut)
            }
        }

        ComboBox {
            id: threadComboBox
            Layout.fillWidth: true
            readonly property var threads: global.threadpoolModule.threads
            model: Object.keys(threads).filter(id => threads[id].mode === RunMode.Debug)
            Accessible.name: qsTr("Debug thread")
            onModelChanged: {
                currentIndex = model.length ? Math.max(0, model.indexOf(root.debugModule.currentThread)) : -1
                root.debugModule.currentThread = model[currentIndex] ?? ""
            }
            onActivated: root.debugModule.currentThread = currentText

            component ThreadRow: RowLayout {
                id: threadRow
                required property string threadId
                readonly property var status: threadComboBox.threads[threadId]?.status

                Label {
                    text: threadRow.threadId
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                IconImage {
                    visible: threadRow.status !== undefined
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    source: threadRow.status === ThreadStatus.Paused ? "qrc:/icon/pause.svg"
                          : threadRow.status === ThreadStatus.Terminating ? "qrc:/icon/stop.svg" : "qrc:/icon/play.svg"
                    color: threadRow.status === ThreadStatus.Paused ? global.theme.general.warningBack3
                         : threadRow.status === ThreadStatus.Terminating ? global.theme.general.dangerBack3 : global.theme.general.successBack3
                }
            }

            contentItem: ThreadRow {
                threadId: threadComboBox.currentText
            }

            delegate: ItemDelegate {
                id: threadItem
                required property int index
                required property string modelData
                width: ListView.view.width
                height: 24
                leftPadding: 11; rightPadding: 6; topPadding: 0; bottomPadding: 0
                text: modelData
                highlighted: threadComboBox.highlightedIndex === index

                contentItem: ThreadRow {
                    threadId: threadItem.modelData
                }
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

            ScrollBar.vertical: ScrollBar {
                id: stackScrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
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
                leftPadding: 11; rightPadding: 6 + stackScrollBar.width
                topPadding: 0; bottomPadding: 0
                focusPolicy: Qt.ClickFocus
                highlighted: ListView.isCurrentItem

                contentItem: RowLayout {
                    spacing: 6

                    Label {
                        text: frame.modelData.name
                        elide: Text.ElideRight
                    }

                    Label {
                        text: frame.modelData.functionName
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: frame.modelData.line >= 0 ? frame.modelData.line + 1 : ""
                    }
                }

                onClicked: {
                    stackView.currentIndex = frame.index
                    if (frame.modelData.line >= 0 && frame.modelData.documentUrl != "") {
                        root.debugModule.setIndex(frame.modelData.documentUrl, frame.modelData.line, 0)
                        root.debugModule.addMarker(modelData.documentUrl, ScintillaMarker.Hint, modelData.line, 1000)
                    }
                }
            }
        }
    }
}

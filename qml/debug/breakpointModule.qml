pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import "../util"

import UniComm

Item {
    id: root
    required property BreakpointModule breakpointModule
    anchors.fill: parent

    Component.onCompleted: listView.reload()

    Connections {
        target: root.breakpointModule
        function onBreakpointsChanged() { Qt.callLater(listView.reload) }
        function onShowBreakpoint(documentUrl, line) {
            Qt.callLater(function() {
                const index = listView.rowIndex(listView.model, documentUrl, line)
                if (index >= 0) {
                    listView.forceActiveFocus()
                    listView.currentIndex = index
                    listView.positionViewAtIndex(index, ListView.Contain)
                }
            })
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 1
                color: global.theme.general.stroke
            }
        }

        Item {
            SplitView.preferredWidth: Math.min(240, root.width / 2)
            clip: true

            Item {
                anchors.fill: parent
                visible: listView.count === 0

                RowLayout {
                    anchors.centerIn: parent

                    Label {
                        text: qsTr("No breakpoints.")
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignVCenter
                    }

                    IconImage {
                        source: "qrc:/icon/activateBreakpoints.svg"
                        color: global.theme.general.fore
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            ListView {
                id: listView
                anchors.fill: parent
                clip: true
                model: []
                currentIndex: -1
                focusPolicy: Qt.ClickFocus
                keyNavigationEnabled: true
                highlightMoveDuration: 0

                function rowIndex(rows, url, line) {
                    return rows.findIndex(row => row.documentUrl.toString() === url.toString() && row.line === line)
                }

                function reload() {
                    const old = model[currentIndex]
                    const rows = root.breakpointModule.breakpoints
                    const index = old ? rowIndex(rows, old.documentUrl, old.line) : -1
                    const next = rows[index]
                    const keepDraft = next && next.line !== undefined && old.condition === next.condition
                    const text = conditionField.text
                    model = Array.from(rows)
                    currentIndex = index
                    if (keepDraft) conditionField.text = text
                }

                ScrollBar.vertical: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: ItemDelegate {
                    id: row
                    required property var modelData
                    required property int index
                    readonly property bool fileRow: modelData.name !== undefined
                    width: listView.width
                    height: 24
                    leftPadding: fileRow ? 6 : 16
                    rightPadding: 6; topPadding: 0; bottomPadding: 0
                    focusPolicy: Qt.ClickFocus
                    highlighted: ListView.isCurrentItem

                    contentItem: RowLayout {
                        spacing: 4

                        Label {
                            text: row.fileRow ? row.modelData.name : row.modelData.line + 1
                            font.bold: row.fileRow
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        CheckBox {
                            visible: !row.fileRow
                            checked: !row.fileRow && row.modelData.enabled
                            padding: 0
                            focusPolicy: Qt.ClickFocus
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24
                            Accessible.name: qsTr("Enabled")

                            onToggled: {
                                listView.currentIndex = row.index
                                root.breakpointModule.breakpointConfigure(row.modelData.documentUrl, row.modelData.line, { enabled: checked })
                            }
                        }
                    }

                    onClicked: {
                        if (!fileRow) listView.currentIndex = index
                    }

                    onDoubleClicked: {
                        const url = modelData.documentUrl
                        if (fileRow) {
                            root.breakpointModule.openDocument(url)
                        } else {
                            const line = modelData.line
                            listView.currentIndex = index
                            root.breakpointModule.setIndex(url, line, 0)
                            root.breakpointModule.addMarker(url, ScintillaMarker.Hint, line, 1000)
                        }
                    }
                }
            }
        }

        ScrollView {
            id: settingsView
            SplitView.fillWidth: true
            clip: true
            padding: 8
            contentWidth: availableWidth

            ColumnLayout {
                id: settings
                width: settingsView.availableWidth
                readonly property var breakpoint: {
                    const row = listView.model[listView.currentIndex]
                    return row && row.line !== undefined ? row : null
                }
                visible: breakpoint !== null

                onBreakpointChanged: {
                    conditionField.text = breakpoint?.condition ?? ""
                }

                Label {
                    text: qsTr("Condition")
                }

                LineEdit {
                    id: conditionField
                    placeholderText: "true"
                    clearable: true
                    tooltip: mainTooltip
                    buttonSize: 24
                    Layout.fillWidth: true

                    onValueCommitted: {
                        const breakpoint = settings.breakpoint
                        if (breakpoint && text !== breakpoint.condition)
                            root.breakpointModule.breakpointConfigure(breakpoint.documentUrl, breakpoint.line, { condition: text })
                    }
                }
            }
        }
    }
}

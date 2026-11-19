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
    readonly property var selectedBreakpoint: listView.model[listView.currentIndex] ?? null
    anchors.fill: parent

    onSelectedBreakpointChanged: conditionField.text = selectedBreakpoint?.condition ?? ""
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

                Keys.onDeletePressed: {
                    const breakpoint = root.selectedBreakpoint
                    if (breakpoint) root.breakpointModule.breakpointDelete(breakpoint.documentUrl, breakpoint.line)
                }

                function rowIndex(rows, url, line) {
                    return rows.findIndex(row => row.documentUrl.toString() === url.toString() && row.line === line)
                }

                function reload() {
                    const old = model[currentIndex]
                    const rows = root.breakpointModule.breakpoints
                    const index = old ? rowIndex(rows, old.documentUrl, old.line) : -1
                    const next = rows[index]
                    const keepDraft = next && old.condition === next.condition
                    const text = conditionField.text
                    model = Array.from(rows)
                    currentIndex = index
                    if (keepDraft) conditionField.text = text
                }

                ScrollBar.vertical: ScrollBar {
                    id: breakpointScrollBar
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: ItemDelegate {
                    id: row
                    required property var modelData
                    required property int index
                    width: listView.width
                    height: 24
                    leftPadding: 11; rightPadding: 6 + breakpointScrollBar.width
                    topPadding: 0; bottomPadding: 0
                    focusPolicy: Qt.ClickFocus
                    highlighted: ListView.isCurrentItem

                    contentItem: RowLayout {
                        spacing: 4

                        Label {
                            text: row.modelData.name
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: row.modelData.line + 1
                        }

                        CheckBox {
                            checked: row.modelData.enabled
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

                    onClicked: listView.currentIndex = index

                    onDoubleClicked: {
                        const url = modelData.documentUrl
                        const line = modelData.line
                        listView.currentIndex = index
                        root.breakpointModule.setIndex(url, line, 0)
                        root.breakpointModule.addMarker(url, ScintillaMarker.Hint, line, 1000)
                    }
                }
            }
        }

        ScrollView {
            id: settingsView
            SplitView.fillWidth: true
            clip: true
            padding: 8
            rightPadding: padding + ScrollBar.vertical.width
            contentWidth: availableWidth

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.minimumSize: Math.min(1, 20 / Math.max(1, ScrollBar.vertical.height))
            ScrollBar.vertical.palette.mid: global.theme.general.stroke
            ScrollBar.vertical.palette.dark: global.theme.general.strokePressed

            ColumnLayout {
                width: settingsView.availableWidth
                visible: root.selectedBreakpoint !== null

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
                        const breakpoint = root.selectedBreakpoint
                        if (breakpoint && text !== breakpoint.condition)
                            root.breakpointModule.breakpointConfigure(breakpoint.documentUrl, breakpoint.line, { condition: text })
                    }
                }
            }
        }
    }
}

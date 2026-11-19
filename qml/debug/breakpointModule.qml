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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: false
            spacing: 0

            Button {
                id: deleteButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Delete breakpoint")
                enabled: root.selectedBreakpoint !== null
                onClicked: root.breakpointModule.breakpointDelete(root.selectedBreakpoint.documentUrl, root.selectedBreakpoint.line)

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = deleteButton.mapToGlobal(point.position)
                        mainTooltip.text = deleteButton.Accessible.name
                    }
                }
            }

            Button {
                id: detailsButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/moreVertical.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Breakpoint details")
                checkable: true
                checked: true
                enabled: listView.count > 0

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = detailsButton.mapToGlobal(point.position)
                        mainTooltip.text = detailsButton.Accessible.name
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: listView.count === 0

            RowLayout {
                anchors.centerIn: parent

                Label {
                    text: qsTr("No breakpoints.")
                    font.pixelSize: 16
                }

                IconImage {
                    source: "qrc:/icon/activateBreakpoints.svg"
                    color: global.theme.general.fore
                }
            }
        }

        SplitView {
            id: splitView
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: listView.count > 0
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
                SplitView.fillWidth: true
                clip: true

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
                property real reveal: detailsButton.checked ? 1 : 0
                SplitView.preferredWidth: splitView.width - Math.min(240, splitView.width / 2)
                SplitView.maximumWidth: reveal === 1 ? splitView.width : Math.min(SplitView.preferredWidth, splitView.width) * reveal
                enabled: detailsButton.checked
                clip: true
                padding: 8
                rightPadding: padding + ScrollBar.vertical.width
                contentWidth: availableWidth

                Behavior on reveal {
                    NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                }

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
}

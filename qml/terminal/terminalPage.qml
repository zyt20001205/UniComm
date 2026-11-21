pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import UniComm

Item {
    id: root
    required property TerminalPage terminalPage
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    TerminalWidget {
        id: terminalWidget
        anchors.fill: parent
        damage: damageItem.checked

        Component.onCompleted: root.terminalPage.terminalWidgetSet(terminalWidget)

        onShowContextMenu: (selection, link, pastable) => {
            mainTooltip.text = ""
            contextMenu.selection = selection
            contextMenu.link = link
            contextMenu.pastable = pastable
            contextMenu.popup()
        }

        onSetToolTip: (position, text) => {
            if (text.length > 0) mainTooltip.position = position
            mainTooltip.text = text
        }

        onDebugDamage: rect => {
            damageOverlay.x = rect.x
            damageOverlay.y = rect.y
            damageOverlay.width = rect.width
            damageOverlay.height = rect.height
        }

        onPreeditChanged: {
            preeditInput.text = preedit.text
            preeditInput.cursorPosition = Math.max(0, preedit.cursor)
        }

        Rectangle {
            visible: terminalWidget.activeFocus && preeditInput.length > 0 && height > 0
            x: Math.min(terminalWidget.cursorRect.x, terminalWidget.width - width)
            y: terminalWidget.cursorRect.y
            width: Math.min(preeditInput.implicitWidth, terminalWidget.width)
            height: terminalWidget.cursorRect.height
            clip: true
            color: global.theme.general.back

            TextInput {
                id: preeditInput
                anchors.fill: parent
                enabled: false
                readOnly: true
                rightPadding: 1
                font: terminalWidget.font
                verticalAlignment: TextInput.AlignVCenter
                color: global.theme.general.fore
                cursorVisible: terminalWidget.preedit.cursor >= 0
                cursorDelegate: Rectangle {
                    visible: preeditInput.cursorVisible
                    width: 1
                    color: terminalWidget.preedit.color.valid ? terminalWidget.preedit.color : preeditInput.color
                }
            }
        }

        Rectangle {
            id: damageOverlay
            visible: damageItem.checked
            color: "transparent"
            border.color: "#ff00ff"
            border.width: 1
            z: 100
        }

        ScrollBar {
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            z: 10

            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
            size: root.terminalPage.vtermWidget.scrollSize
            position: root.terminalPage.vtermWidget.scrollPosition

            onPositionChanged: {
                if (pressed) root.terminalPage.vtermWidget.scrollPosition = position
            }
        }
    }

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        focus: false
        property string selection
        property url link
        property bool pastable

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            enabled: contextMenu.selection.length > 0
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.copyToClipboard(contextMenu.selection)
        }

        MenuItem {
            text: qsTr("Paste")
            enabled: contextMenu.pastable
            icon.source: "qrc:/icon/paste.svg"
            icon.width: 16; icon.height: 16

            onTriggered: terminalWidget.paste()
        }

        Menu {
            title: qsTr("Link")
            enabled: contextMenu.link.toString().length > 0
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Copy")
                icon.source: "qrc:/icon/copy.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.fileModule.copyToClipboard(contextMenu.link.toString())
            }

            MenuItem {
                text: qsTr("Open")
                icon.source: "qrc:/icon/link.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.documentOpen(contextMenu.link)
            }

            MenuItem {
                text: qsTr("Open Externally")
                icon.source: "qrc:/icon/open.svg"
                icon.width: 16; icon.height: 16

                onTriggered: Qt.openUrlExternally(contextMenu.link)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: damageItem
            text: qsTr("Repaint Region")
            checkable: true
            icon.source: "qrc:/icon/squareHint.svg"
            icon.width: 16; icon.height: 16
        }
    }
}

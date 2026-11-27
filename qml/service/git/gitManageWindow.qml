pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../util"

Item {
    id: root
    property alias pageIndex: navigationView.currentIndex

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        ListView {
            id: navigationView
            Layout.preferredWidth: 180; Layout.fillHeight: true
            clip: true
            currentIndex: 0
            focusPolicy: Qt.ClickFocus
            keyNavigationEnabled: true
            model: [
                {title: qsTr("Remotes"), icon: "qrc:/icon/cloud.svg"},
                {title: qsTr("Proxy"), icon: "qrc:/icon/plugConnected.svg"}
            ]

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: ItemDelegate {
                required property int index
                required property var modelData
                width: navigationView.width; height: 28
                leftPadding: 11; rightPadding: 6 + scrollBar.width
                topPadding: 0; bottomPadding: 0
                text: modelData.title
                icon.source: modelData.icon
                icon.width: 16; icon.height: 16
                focusPolicy: Qt.ClickFocus
                highlighted: ListView.isCurrentItem

                onClicked: navigationView.currentIndex = index
            }
        }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: navigationView.currentIndex

            ColumnLayout {
                spacing: 0

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 0

                    Button {
                        id: addButton
                        flat: true
                        enabled: global.serviceModule.git.hasRepo
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        icon.source: "qrc:/icon/addSquare.svg"
                        icon.width: 16; icon.height: 16
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        Accessible.name: qsTr("Add remote")
                        onClicked: addDialog.open()

                        HoverHandler {
                            onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                            onPointChanged: {
                                mainTooltip.position = addButton.mapToGlobal(point.position)
                                mainTooltip.text = addButton.Accessible.name
                            }
                        }
                    }

                    Button {
                        id: deleteButton
                        flat: true
                        checkable: true
                        enabled: global.serviceModule.git.hasRepo && remoteView.currentItem !== null
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                        icon.width: 16; icon.height: 16
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        Accessible.name: checked ? qsTr("Confirm deletion") : qsTr("Delete remote")
                        onToggled: { if (!checked) global.serviceModule.git.removeRemote(remoteView.currentItem.modelData.name) }

                        Timer {
                            interval: 1000
                            running: deleteButton.checked
                            onTriggered: deleteButton.checked = false
                        }

                        HoverHandler {
                            onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                            onPointChanged: {
                                mainTooltip.position = deleteButton.mapToGlobal(point.position)
                                mainTooltip.text = deleteButton.Accessible.name
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    Layout.leftMargin: 11; Layout.rightMargin: 6 + remoteScrollBar.width
                    spacing: 8

                    Label {
                        id: nameHeader
                        Layout.preferredWidth: 120
                        text: qsTr("Name")
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true; Layout.preferredWidth: 0
                        text: qsTr("Fetch URL")
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true; Layout.preferredWidth: 0
                        text: qsTr("Push URL")
                        font.bold: true
                    }
                }

                ListView {
                    id: remoteView
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true
                    focusPolicy: Qt.ClickFocus
                    keyNavigationEnabled: true
                    model: global.serviceModule.git.remotes
                    onCurrentItemChanged: deleteButton.checked = false

                    ScrollBar.vertical: ScrollBar {
                        id: remoteScrollBar
                        minimumSize: Math.min(1, 20 / Math.max(1, height))
                        palette {
                            mid: global.theme.general.stroke
                            dark: global.theme.general.strokePressed
                        }
                    }

                    delegate: ItemDelegate {
                        id: remote
                        required property int index
                        required property var modelData
                        width: remoteView.width
                        height: Math.max(28, implicitContentHeight)
                        leftPadding: 11; rightPadding: 6 + remoteScrollBar.width
                        topPadding: 0; bottomPadding: 0
                        focusPolicy: Qt.ClickFocus
                        highlighted: ListView.isCurrentItem
                        onClicked: remoteView.currentIndex = index

                        contentItem: RowLayout {
                            spacing: 8

                            Label {
                                Layout.preferredWidth: nameHeader.width
                                text: remote.modelData.name
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true; Layout.preferredWidth: 0
                                text: remote.modelData.fetchUrls.join("\n")
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true; Layout.preferredWidth: 0
                                text: remote.modelData.pushUrls.join("\n")
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                Label { text: qsTr("Local Proxy") }
                LineEdit {
                    id: localProxy
                    Layout.fillWidth: true
                    enabled: global.serviceModule.git.hasRepo
                    text: global.serviceModule.git.config.proxy.local
                    clearable: true
                    placeholderText: "http://127.0.0.1:7890"
                }
                Label { text: qsTr("Global Proxy") }
                LineEdit {
                    id: globalProxy
                    Layout.fillWidth: true
                    text: global.serviceModule.git.config.proxy.global
                    clearable: true
                    placeholderText: "http://127.0.0.1:7890"
                }
                Item { Layout.fillHeight: true }
                Button {
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("Apply")
                    enabled: (localProxy.enabled && localProxy.text !== global.serviceModule.git.config.proxy.local)
                             || globalProxy.text !== global.serviceModule.git.config.proxy.global
                    onClicked: global.serviceModule.git.proxySet(localProxy.text, globalProxy.text)
                }
            }
        }
    }

    Dialog {
        id: addDialog
        parent: mainScreen
        anchors.centerIn: parent
        popupType: Popup.Item
        modal: true
        width: 480
        title: qsTr("Add remote")
        property bool pending: false
        closePolicy: pending ? Popup.NoAutoClose : Popup.CloseOnEscape

        onAboutToShow: {
            nameField.text = remoteView.count ? "" : "origin"
            urlField.clear()
            errorLabel.text = ""
        }
        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
            if (nameField.length) urlField.forceActiveFocus()
            else nameField.forceActiveFocus()
        }
        onClosed: mainOverlay.widgetCount -= 1

        ColumnLayout {
            width: parent.width
            enabled: !addDialog.pending

            LineEdit {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                clearable: true
                onAccepted: urlField.forceActiveFocus()
            }
            LineEdit {
                id: urlField
                Layout.fillWidth: true
                placeholderText: qsTr("URL")
                clearable: true
                onAccepted: { if (submitButton.enabled) submitButton.clicked() }
            }
            Label {
                id: errorLabel
                Layout.fillWidth: true
                visible: text.length > 0
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                color: global.theme.general.dangerFore2
            }
        }

        footer: DialogButtonBox {
            enabled: !addDialog.pending
            standardButtons: DialogButtonBox.Cancel
            Button {
                id: submitButton
                text: qsTr("Add")
                enabled: nameField.length > 0 && urlField.length > 0
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: {
                    addDialog.pending = true
                    global.serviceModule.git.addRemote(nameField.text, urlField.text)
                }
            }
        }
        Connections {
            target: global.serviceModule.git
            function onRemoteAddFinished(success, error) {
                addDialog.pending = false
                if (success) addDialog.accept()
                else errorLabel.text = error
            }
        }
    }
}

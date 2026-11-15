import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    RowLayout {
        anchors.fill: parent

        Button {
            id: fileMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "File"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = fileMenuButton.mapToGlobal(0, fileMenuButton.height);
                const localPos = fileMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                fileMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: editMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Edit"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = editMenuButton.mapToGlobal(0, editMenuButton.height);
                const localPos = editMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                editMenu.popup(localPos.x, localPos.y)
            }
        }
        
        Button {
            id: viewMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "View"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = viewMenuButton.mapToGlobal(0, viewMenuButton.height);
                const localPos = viewMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                viewMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: navMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Nav"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = navMenuButton.mapToGlobal(0, navMenuButton.height);
                const localPos = navMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                navMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: codeMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Code"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = codeMenuButton.mapToGlobal(0, codeMenuButton.height);
                const localPos = codeMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                codeMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: execMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Exec"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = execMenuButton.mapToGlobal(0, execMenuButton.height);
                const localPos = execMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                execMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: gitMenuButton
            enabled: global.gitModule.gitEnabled
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Git"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = gitMenuButton.mapToGlobal(0, gitMenuButton.height);
                const localPos = gitMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                gitMenu.popup(localPos.x, localPos.y)
            }
        }

        Item {
            Layout.fillWidth: true; Layout.preferredHeight: 24
        }

        Button {
            id: themeButton
            Layout.preferredWidth: 24; Layout.preferredHeight: 24
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            checkable: true
            icon.source: checked ? "qrc:/icon/checkmark.svg" :
                global.theme === 0 ? "qrc:/icon/themeDark.svg" : "qrc:/icon/themeLight.svg"
            icon.width: 16; icon.height: 16

            onToggled: {
                if (!checked) {
                    menuModule.themeSet(global.theme === 0 ? 1 : 0)
                }
            }

            Timer {
                interval: 1000
                running: parent.checked
                onTriggered: parent.checked = false
            }
        }
    }
}
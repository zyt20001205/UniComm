import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

TextField {
    id: control

    property bool revealable: false
    property bool clearable: false
    property QtObject tooltip: mainToolTip
    enum FileMode { OpenFile, OpenFiles, SaveFile, Folder }
    property int fileMode: LineEdit.OpenFile
    property bool browsable: fileMode === LineEdit.Folder
    property alias nameFilters: fileDialog.nameFilters
    property alias currentFolder: fileDialog.currentFolder
    property alias selectedFile: fileDialog.selectedFile
    property alias title: fileDialog.title
    property int buttonSize: 24
    property int iconSize: 16

    signal valueCommitted()

    function commitPath(path: url) {
        text = path.toString()
        forceActiveFocus()
        valueCommitted()
    }

    rightPadding: padding + actions.implicitWidth
    echoMode: revealable && !revealButton.checked ? TextInput.Password : TextInput.Normal

    onEditingFinished: valueCommitted()
    onActiveFocusChanged: {
        if (!activeFocus) revealButton.checked = false
    }

    RowLayout {
        id: actions
        anchors.right: parent.right
        anchors.rightMargin: control.padding
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Button {
            id: revealButton
            visible: control.revealable
            checkable: true
            flat: true
            focusPolicy: Qt.NoFocus
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            icon.source: checked ? "qrc:/icon/hide.svg" : "qrc:/icon/eye.svg"
            icon.width: control.iconSize; icon.height: control.iconSize
            Layout.preferredWidth: control.buttonSize; Layout.preferredHeight: control.buttonSize
            Accessible.name: checked ? qsTr("Hide password") : qsTr("Show password")

            HoverHandler {
                onHoveredChanged: { if (!hovered) control.tooltip.text = "" }
                onPointChanged: {
                    control.tooltip.position = parent.mapToGlobal(point.position)
                    control.tooltip.text = parent.Accessible.name
                }
            }
        }

        Button {
            visible: control.clearable && control.length > 0 && !control.readOnly
            flat: true
            focusPolicy: Qt.NoFocus
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            icon.source: "qrc:/icon/dismiss.svg"
            icon.width: control.iconSize; icon.height: control.iconSize
            Layout.preferredWidth: control.buttonSize; Layout.preferredHeight: control.buttonSize
            Accessible.name: qsTr("Clear")

            onClicked: {
                control.clear()
                if (control.fileMode === LineEdit.Folder) folderDialog.selectedFolder = ""
                else if (control.browsable) fileDialog.selectedFile = ""
                control.forceActiveFocus()
                control.valueCommitted()
            }

            HoverHandler {
                onHoveredChanged: { if (!hovered) control.tooltip.text = "" }
                onPointChanged: {
                    control.tooltip.position = parent.mapToGlobal(point.position)
                    control.tooltip.text = parent.Accessible.name
                }
            }
        }

        Button {
            visible: control.browsable
            enabled: !control.readOnly
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            icon.source: {
                switch (control.fileMode) {
                case LineEdit.OpenFiles: return "qrc:/icon/documentMultiple.svg"
                case LineEdit.SaveFile: return "qrc:/icon/documentSave.svg"
                case LineEdit.Folder: return "qrc:/icon/folder.svg"
                default: return "qrc:/icon/document.svg"
                }
            }
            icon.width: control.iconSize; icon.height: control.iconSize
            Layout.preferredWidth: control.buttonSize; Layout.preferredHeight: control.buttonSize
            Accessible.name: qsTr("Browse")

            onClicked: {
                if (control.fileMode === LineEdit.Folder) {
                    if (control.text) folderDialog.currentFolder = control.text
                    folderDialog.open()
                    return
                }
                if (control.text) {
                    const url = control.text
                    fileDialog.selectedFile = url
                    fileDialog.currentFolder = url.substring(0, url.lastIndexOf('/'))
                }
                fileDialog.open()
            }

            HoverHandler {
                onHoveredChanged: { if (!hovered) control.tooltip.text = "" }
                onPointChanged: {
                    control.tooltip.position = parent.mapToGlobal(point.position)
                    control.tooltip.text = parent.Accessible.name
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        fileMode: control.fileMode === LineEdit.Folder ? FileDialog.OpenFile : control.fileMode

        onAccepted: control.commitPath(selectedFile)
    }

    FolderDialog {
        id: folderDialog
        title: control.title
        currentFolder: control.currentFolder
        onAccepted: control.commitPath(selectedFolder)
    }
}

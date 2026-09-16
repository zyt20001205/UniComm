import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            id: toolBar
            Layout.fillWidth: true
            spacing: 0

            Item {
                Layout.fillWidth: true
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    const row = terminalModule.terminalAdd()
                    tableView.selectedRow = row
                    programTextField.clear()
                    argumentsTextField.clear()
                    workingDirectoryTextField.clear()
                    environmentTextField.clear()
                    tableView.positionViewAtRow(row, TableView.Contain, 0, Qt.rect(0, 0, 0, 0))
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                flat: true
                icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/subtract.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onToggled: {
                    if (!checked) terminalModule.terminalDelete(tableView.selectedRow)
                }

                Timer {
                    interval: 1000
                    running: parent.checked
                    onTriggered: parent.checked = false
                }
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            VerticalHeaderView {
                id: verticalHeaderView
                anchors.left: parent.left
                width: 32; height: parent.height
                syncView: tableView
                clip: true
                interactive: false
                movableRows: true
                delegate: VerticalHeaderViewDelegate {
                    implicitWidth: verticalHeaderView.width; implicitHeight: 32
                    padding: 0

                    contentItem: Rectangle {
                        width: 32; height: 32
                        color: global.back

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            color: global.fore
                            source: "qrc:/icon/drag.svg"
                        }
                    }

                    HoverHandler {
                        onHoveredChanged: cursorShape = Qt.OpenHandCursor
                    }
                }
                property var moves: []

                Rectangle {
                    anchors.fill: parent
                    color: global.stroke
                }

                Timer {
                    id: timer
                    interval: 10
                    onTriggered: {
                        if (verticalHeaderView.moves.length === 0) return
                        let index = -1
                        let distance = -1
                        for (let i = 0; i < verticalHeaderView.moves.length; ++i) {
                            const move = verticalHeaderView.moves[i]
                            const currentDistance = Math.abs(move.oldVisualIndex - move.newVisualIndex)
                            if (currentDistance > distance) {
                                distance = currentDistance
                                index = i
                            }
                        }
                        const move = verticalHeaderView.moves[index]
                        terminalModule.terminalSwap(move.oldVisualIndex, move.newVisualIndex)
                        verticalHeaderView.clearRowReordering()
                        tableView.clearRowReordering()
                        verticalHeaderView.moves = []
                    }
                }

                onRowMoved: (logicalIndex, oldVisualIndex, newVisualIndex) => {
                    moves.push({logicalIndex, oldVisualIndex, newVisualIndex})
                    timer.restart()
                }
            }

            TableView {
                id: tableView
                anchors.left: verticalHeaderView.right; anchors.right: parent.right
                anchors.top: parent.top; anchors.bottom: parent.bottom
                alternatingRows: false
                clip: true
                editTriggers: TableView.NoEditTriggers
                rowSpacing: 1
                model: terminalModel
                contentWidth: width
                property int hoveredRow: -1
                property int selectedRow: -1

                ScrollBar.vertical: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: global.stroke
                }

                delegate: Item {
                    implicitWidth: tableView.width
                    implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        color: global.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: global.backHover
                        opacity: tableView.hoveredRow === row ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: tableView.selectedRow === row ? global.backSelected : "transparent"
                    }

                    Label {
                        anchors.fill: parent
                        leftPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.session.name || ""
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        onHoveredChanged: {
                            if (hovered) {
                                tableView.hoveredRow = row
                            } else if (tableView.hoveredRow === row) {
                                tableView.hoveredRow = -1
                            }
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onSingleTapped: {
                            tableView.selectedRow = row
                            const session = model.session
                            programTextField.text = session.program
                            argumentsTextField.text = session.arguments
                            workingDirectoryTextField.text = session.workingDirectory
                            environmentTextField.text = session.environment
                        }

                        onDoubleTapped: {
                            tableView.selectedRow = row
                            tableView.edit(tableView.index(row, column))
                        }
                    }

                    TableView.editDelegate: TextField {
                        anchors.fill: parent
                        leftPadding: 6
                        rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        text: session.name || ""
                        selectByMouse: true
                        background: Rectangle {
                            color: global.backSelected
                            radius: 6
                        }

                        Component.onCompleted: {
                            forceActiveFocus()
                            selectAll()
                        }

                        TableView.onCommit: {
                            const index = terminalModel.index(row, column)
                            const session = terminalModel.data(index, Qt.UserRole + 2)
                            session.name = text.trim()
                            terminalModel.setData(index, session, Qt.UserRole + 2)
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onSingleTapped: tableView.closeEditor()
                }
            }
        }

        ColumnLayout {
            visible: tableView.selectedRow !== -1
            Layout.fillWidth: true

            Label {
                text: qsTr("Program")
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: programTextField
                    Layout.fillWidth: true

                    onEditingFinished: {
                        const index = terminalModel.index(tableView.selectedRow, 0)
                        const session = terminalModel.data(index, Qt.UserRole + 2)
                        session.program = programTextField.text
                        terminalModel.setData(index, session, Qt.UserRole + 2)
                    }
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    icon.source: "qrc:/icon/folder.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 32; Layout.preferredHeight: 32

                    onClicked: {
                        if (programTextField.text) {
                            const url = programTextField.text
                            programDialog.selectedFile = url
                            programDialog.currentFolder = url.substring(0, url.lastIndexOf('/'))
                        }
                        programDialog.open()
                    }
                }

                FileDialog {
                    id: programDialog
                    fileMode: FileDialog.OpenFile
                    nameFilters: ["Executable files (*.exe)", "All files (*)"]
                    onAccepted: {
                        programTextField.text = selectedFile
                        const index = terminalModel.index(tableView.selectedRow, 0)
                        const session = terminalModel.data(index, Qt.UserRole + 2)
                        session.program = selectedFile
                        terminalModel.setData(index, session, Qt.UserRole + 2)
                    }
                }
            }

            Label {
                text: qsTr("Arguments")
                Layout.fillWidth: true
            }

            TextField {
                id: argumentsTextField
                Layout.fillWidth: true

                onEditingFinished: {
                    const index = terminalModel.index(tableView.selectedRow, 0)
                    const session = terminalModel.data(index, Qt.UserRole + 2)
                    session.arguments = argumentsTextField.text
                    terminalModel.setData(index, session, Qt.UserRole + 2)
                }
            }

            Label {
                text: qsTr("Working Directory")
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: workingDirectoryTextField
                    Layout.fillWidth: true

                    onEditingFinished: {
                        const index = terminalModel.index(tableView.selectedRow, 0)
                        const session = terminalModel.data(index, Qt.UserRole + 2)
                        session.workingDirectory = workingDirectoryTextField.text
                        terminalModel.setData(index, session, Qt.UserRole + 2)
                    }
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    icon.source: "qrc:/icon/folder.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 32; Layout.preferredHeight: 32

                    onClicked: {
                        if (workingDirectoryTextField.text) {
                            workingDirectoryDialog.currentFolder = workingDirectoryTextField.text
                        }
                        workingDirectoryDialog.open()
                    }
                }

                FolderDialog {
                    id: workingDirectoryDialog

                    onAccepted: {
                        workingDirectoryTextField.text = selectedFolder
                        const index = terminalModel.index(tableView.selectedRow, 0)
                        const session = terminalModel.data(index, Qt.UserRole + 2)
                        session.workingDirectory = selectedFolder
                        terminalModel.setData(index, session, Qt.UserRole + 2)
                    }
                }
            }

            Label {
                text: qsTr("Environment")
                Layout.fillWidth: true
            }

            TextField {
                id: environmentTextField
                Layout.fillWidth: true
                placeholderText: "NAME=value;NAME2=value"

                onEditingFinished: {
                    const index = terminalModel.index(tableView.selectedRow, 0)
                    const session = terminalModel.data(index, Qt.UserRole + 2)
                    session.environment = text
                    terminalModel.setData(index, session, Qt.UserRole + 2)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight

            Button {
                text: qsTr("Apply")
            }

            Button {
                text: qsTr("Save and Exit")

                onClicked: terminalModule.terminalSave()
            }
        }
    }
}

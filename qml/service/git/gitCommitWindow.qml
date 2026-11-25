import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    id: root
    required property GitModule gitModule
    required property var workingTreeModel
    required property var indexModel
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Label {
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    font.bold: true
                    text: qsTr("Working Tree")
                    Layout.fillWidth: true
                }

                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 2
                        border.color: global.theme.general.stroke
                        radius: 6
                    }

                    TreeView {
                        id: workingTreeView
                        anchors.fill: parent
                        anchors.margins: 6
                        clip: true
                        model: root.workingTreeModel
                        property int selectedRow: -1
                        property string documentUrl: ""

                        ScrollBar.vertical: ScrollBar {
                            minimumSize: Math.min(1, 20 / Math.max(1, height))
                            policy: ScrollBar.AsNeeded
                            palette {
                                mid: global.theme.general.stroke
                                dark: global.theme.general.strokePressed
                            }
                        }

                        delegate: Item {
                            implicitWidth: workingTreeView.width; implicitHeight: 24
                            required property bool isTreeNode
                            required property bool expanded
                            required property bool hasChildren
                            required property int depth
                            required property int row

                            Rectangle {
                                anchors.fill: parent
                                radius: 6
                                color: global.theme.general.backHover
                                opacity: hoverHandler.hovered ? 1 : 0
                                Behavior on opacity {
                                    NumberAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: 6
                                color: workingTreeView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                            }

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0

                                Item {
                                    Layout.preferredWidth: depth * 24; Layout.preferredHeight: 24
                                }

                                Item {
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    IconImage {
                                        anchors.centerIn: parent
                                        width: 16; height: 16
                                        source: expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                        color: global.theme.general.fore
                                        visible: isTreeNode && hasChildren
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        gesturePolicy: TapHandler.DragWithinBounds

                                        onTapped: {
                                            workingTreeView.selectedRow = row
                                            if (isTreeNode && hasChildren) {
                                                workingTreeView.toggleExpanded(row)
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    Image {
                                        anchors.centerIn: parent
                                        width: 16; height: 16
                                        source: !expanded ? model.decoration
                                            : model.decoration == "qrc:/icon/fileTypeFolder.svg" ? "qrc:/icon/fileTypeFolderOpen.svg"
                                                : model.decoration
                                    }
                                }

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: model.display
                                    elide: Text.ElideRight
                                    color: GitStyle.fileStatusColor(model.status)
                                    Layout.fillWidth: true; Layout.preferredHeight: 24
                                }
                            }

                            HoverHandler {
                                id: hoverHandler

                                onHoveredChanged: {
                                    if (!hovered) {
                                        mainTooltip.text = ""
                                    }
                                }
                                onPointChanged: {
                                    if (!(isTreeNode && hasChildren)) {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = Qt.enumValueToString(GitFileStatus.Value, model.status)
                                    }
                                }
                            }

                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                gesturePolicy: TapHandler.DragWithinBounds

                                onTapped: {
                                    workingTreeView.selectedRow = row
                                    workingTreeView.documentUrl = model.documentUrl
                                }
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton

                            onTapped: {
                                workingTreeView.selectedRow = -1
                                workingTreeView.documentUrl = ""
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 32
                Layout.alignment: Qt.AlignVCenter

                Button {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignHCenter
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    highlighted: true
                    icon.source: "qrc:/icon/addSquare.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Add all to Index")
                        }
                    }

                    onClicked: {
                        root.gitModule.add()
                        workingTreeView.selectedRow = -1
                        workingTreeView.documentUrl = ""
                    }
                }

                Button {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignHCenter
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    enabled: workingTreeView.selectedRow !== -1
                    flat: true
                    icon.source: "qrc:/icon/addSquare.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Add selected to Index")
                        }
                    }

                    onClicked: {
                        root.gitModule.add(workingTreeView.documentUrl)
                        workingTreeView.selectedRow = -1
                        workingTreeView.documentUrl = ""
                    }
                }

                Button {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignHCenter
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    enabled: indexTreeView.selectedRow !== -1
                    flat: true
                    icon.source: "qrc:/icon/arrowLeft.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Restore selected to Working tree")
                        }
                    }

                    onClicked: {
                        root.gitModule.restore(indexTreeView.documentUrl, GitRestoreMode.Staged)
                        indexTreeView.selectedRow = -1
                        indexTreeView.documentUrl = ""
                    }
                }

                Button {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignHCenter
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    highlighted: true
                    icon.source: "qrc:/icon/arrowLeft.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Restore all to Working tree")
                        }
                    }

                    onClicked: {
                        root.gitModule.restore("", GitRestoreMode.Staged)
                        indexTreeView.selectedRow = -1
                        indexTreeView.documentUrl = ""
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Label {
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    font.bold: true
                    text: qsTr("Index")
                    Layout.fillWidth: true
                }

                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 2
                        border.color: global.theme.general.stroke
                        radius: 6
                    }

                    TreeView {
                        id: indexTreeView
                        anchors.fill: parent
                        anchors.margins: 6
                        clip: true
                        model: root.indexModel
                        property int selectedRow: -1
                        property string documentUrl: ""

                        ScrollBar.vertical: ScrollBar {
                            minimumSize: Math.min(1, 20 / Math.max(1, height))
                            policy: ScrollBar.AsNeeded
                            palette {
                                mid: global.theme.general.stroke
                                dark: global.theme.general.strokePressed
                            }
                        }

                        delegate: Item {
                            implicitWidth: indexTreeView.width; implicitHeight: 24
                            required property bool isTreeNode
                            required property bool expanded
                            required property bool hasChildren
                            required property int depth
                            required property int row

                            Rectangle {
                                anchors.fill: parent
                                radius: 6
                                color: global.theme.general.backHover
                                opacity: hoverHandler.hovered ? 1 : 0
                                Behavior on opacity {
                                    NumberAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: 6
                                color: indexTreeView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                            }

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0

                                Item {
                                    Layout.preferredWidth: depth * 24; Layout.preferredHeight: 24
                                }

                                Item {
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    IconImage {
                                        anchors.centerIn: parent
                                        width: 16; height: 16
                                        source: expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                        color: global.theme.general.fore
                                        visible: isTreeNode && hasChildren
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        gesturePolicy: TapHandler.DragWithinBounds

                                        onTapped: {
                                            indexTreeView.selectedRow = row
                                            if (isTreeNode && hasChildren) {
                                                indexTreeView.toggleExpanded(row)
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    Image {
                                        anchors.centerIn: parent
                                        width: 16; height: 16
                                        source: !expanded ? model.decoration
                                            : model.decoration == "qrc:/icon/fileTypeFolder.svg" ? "qrc:/icon/fileTypeFolderOpen.svg"
                                                : model.decoration
                                    }
                                }

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: model.display
                                    elide: Text.ElideRight
                                    color: GitStyle.fileStatusColor(model.status)
                                    Layout.fillWidth: true; Layout.preferredHeight: 24
                                }
                            }

                            HoverHandler {
                                id: hoverHandler

                                onHoveredChanged: {
                                    if (!hovered) {
                                        mainTooltip.text = ""
                                    }
                                }
                                onPointChanged: {
                                    if (!(isTreeNode && hasChildren)) {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = Qt.enumValueToString(GitFileStatus.Value, model.status)
                                    }
                                }
                            }

                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                gesturePolicy: TapHandler.DragWithinBounds

                                onTapped: {
                                    indexTreeView.selectedRow = row
                                    indexTreeView.documentUrl = model.documentUrl
                                }
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton

                            onTapped: {
                                indexTreeView.selectedRow = -1
                                indexTreeView.documentUrl = ""
                            }
                        }
                    }
                }
            }
        }

        TextArea {
            id: commitTextArea
            textFormat: TextEdit.PlainText
            verticalAlignment: TextEdit.AlignTop
            wrapMode: Text.Wrap
            placeholderText: qsTr("Commit Message")
            Layout.fillWidth: true; Layout.preferredHeight: 100
        }

        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignRight

            Button {
                enabled: commitTextArea.text && !root.indexModel.empty
                text: qsTr("Commit")

                onClicked: root.gitModule.gitCommit(commitTextArea.text)
            }
        }
    }

    Connections {
        target: root.gitModule

        function onResetCommit() {
            workingTreeView.selectedRow = -1
            workingTreeView.documentUrl = ""
            indexTreeView.selectedRow = -1
            indexTreeView.documentUrl = ""
            commitTextArea.text = ""
        }

        function onExpandChanges() {
            workingTreeView.expandRecursively()
            indexTreeView.expandRecursively()
        }
    }
}

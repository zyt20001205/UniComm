import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    id: rootItem
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
                        model: workingTreeModel
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
                                    color: {
                                        switch (model.status) {
                                            case GitFileStatus.Untracked:
                                                return global.theme.general.successFore3
                                            case GitFileStatus.Modified:
                                                return global.theme.general.brandBack
                                            case GitFileStatus.Added:
                                                return global.theme.general.successFore2
                                            case GitFileStatus.Deleted:
                                                return global.theme.general.stroke
                                            case GitFileStatus.Renamed:
                                                return global.theme.general.warningFore3
                                            case GitFileStatus.Unmerged:
                                                return global.theme.general.dangerFore3
                                            default:
                                                return global.theme.general.fore
                                        }
                                    }
                                    Layout.fillWidth: true; Layout.preferredHeight: 24
                                }
                            }

                            HoverHandler {
                                id: hoverHandler

                                onHoveredChanged: {
                                    if (!hovered) {
                                        mainToolTip.text = ""
                                    }
                                }
                                onPointChanged: {
                                    if (!(isTreeNode && hasChildren)) {
                                        mainToolTip.position = parent.mapToGlobal(point.position)
                                        mainToolTip.text = Qt.enumValueToString(GitFileStatus.Value, model.status)
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
                    icon.source: "qrc:/icon/arrowRight.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Add all to Index")
                        }
                    }

                    onClicked: {
                        gitModule.gitAdd()
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
                    icon.source: "qrc:/icon/arrowRight.svg"
                    icon.width: 16; icon.height: 16

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Add selected to Index")
                        }
                    }

                    onClicked: {
                        gitModule.gitAdd(workingTreeView.documentUrl)
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
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Restore selected to Working tree")
                        }
                    }

                    onClicked: {
                        gitModule.gitRestore(indexTreeView.documentUrl, GitRestoreMode.Staged)
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
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Restore all to Working tree")
                        }
                    }

                    onClicked: {
                        gitModule.gitRestore("", GitRestoreMode.Staged)
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
                        model: indexModel
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
                                    color: {
                                        switch (model.status) {
                                            case GitFileStatus.Untracked:
                                                return global.theme.general.successFore3
                                            case GitFileStatus.Modified:
                                                return global.theme.general.brandBack
                                            case GitFileStatus.Added:
                                                return global.theme.general.successFore2
                                            case GitFileStatus.Deleted:
                                                return global.theme.general.stroke
                                            case GitFileStatus.Renamed:
                                                return global.theme.general.warningFore3
                                            case GitFileStatus.Unmerged:
                                                return global.theme.general.dangerFore3
                                            default:
                                                return global.theme.general.fore
                                        }
                                    }
                                    Layout.fillWidth: true; Layout.preferredHeight: 24
                                }
                            }

                            HoverHandler {
                                id: hoverHandler

                                onHoveredChanged: {
                                    if (!hovered) {
                                        mainToolTip.text = ""
                                    }
                                }
                                onPointChanged: {
                                    if (!(isTreeNode && hasChildren)) {
                                        mainToolTip.position = parent.mapToGlobal(point.position)
                                        mainToolTip.text = Qt.enumValueToString(GitFileStatus.Value, model.status)
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
                enabled: commitTextArea.text && !indexModel.empty
                text: qsTr("Commit")

                onClicked: gitModule.gitCommit(commitTextArea.text)
            }
        }
    }

    function reset() {
        workingTreeView.selectedRow = -1
        workingTreeView.documentUrl = ""
        indexTreeView.selectedRow = -1
        indexTreeView.documentUrl = ""
        commitTextArea.text = ""
    }

    function workingTreeExpand() {
        for (let i = 0; i < workingTreeView.rows; ++i) {
            workingTreeView.expandRecursively(i)
        }
    }

    function indexExpand() {
        for (let i = 0; i < indexTreeView.rows; ++i) {
            indexTreeView.expandRecursively(i)
        }
    }
}

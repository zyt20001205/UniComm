import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
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
                        border.color: global.stroke
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
                                mid: global.stroke
                                dark: global.strokePressed
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
                                color: global.backHover
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
                                color: workingTreeView.selectedRow === row ? global.backSelected : "transparent"
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
                                        color: global.fore
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
                                            case 0:
                                                return global.successFore3
                                            case 3:
                                                return global.brandBack
                                            case 5:
                                                return global.successFore2
                                            case 6:
                                                return global.stroke
                                            case 7:
                                                return global.warningFore3
                                            case 9:
                                                return global.dangerFore3
                                            default:
                                                return global.fore
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
                                        switch (model.status) {
                                            case 0:
                                                mainToolTip.text = "Untracked"
                                                break
                                            case 3:
                                                mainToolTip.text = "Modified"
                                                break
                                            case 5:
                                                mainToolTip.text = "Added"
                                                break
                                            case 6:
                                                mainToolTip.text = "Deleted"
                                                break
                                            case 7:
                                                mainToolTip.text = "Renamed"
                                                break
                                            case 9:
                                                mainToolTip.text = "Unmerged"
                                                break
                                            default:
                                                mainToolTip.text = "contact author: unsupported status (" + model.status + ")"
                                        }
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
                        gitModule.gitRestore(indexTreeView.documentUrl, 1)
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
                        gitModule.gitRestore("", 1)
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
                        border.color: global.stroke
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
                                mid: global.stroke
                                dark: global.strokePressed
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
                                color: global.backHover
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
                                color: indexTreeView.selectedRow === row ? global.backSelected : "transparent"
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
                                        color: global.fore
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
                                            case 0:
                                                return global.successFore3
                                            case 3:
                                                return global.brandBack
                                            case 5:
                                                return global.successFore2
                                            case 6:
                                                return global.stroke
                                            case 7:
                                                return global.warningFore3
                                            case 9:
                                                return global.dangerFore3
                                            default:
                                                return global.fore
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
                                        switch (model.status) {
                                            case 0:
                                                mainToolTip.text = "Untracked"
                                                break
                                            case 3:
                                                mainToolTip.text = "Modified"
                                                break
                                            case 5:
                                                mainToolTip.text = "Added"
                                                break
                                            case 6:
                                                mainToolTip.text = "Deleted"
                                                break
                                            case 7:
                                                mainToolTip.text = "Renamed"
                                                break
                                            case 9:
                                                mainToolTip.text = "Unmerged"
                                                break
                                            default:
                                                mainToolTip.text = "contact author: unsupported status (" + model.status + ")"
                                        }
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

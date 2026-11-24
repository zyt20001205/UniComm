pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    id: root
    readonly property ExplorerModule explorerModule: global.explorerModule
    required property var modelRootIndex
    required property url modelRootUrl
    anchors.fill: parent

    Connections {
        target: root.explorerModule
        function onClearSelection() { treeView.selectedRow = -1 }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            id: toolBar
            Layout.fillWidth: true
            spacing: 0

            Rectangle {
                id: rootTarget
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                radius: 6
                color: dropArea.directory === root.modelRootUrl.toString() ? global.theme.general.backSelected : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    visible: dropArea.containsDrag
                    spacing: 4

                    IconImage {
                        source: "qrc:/icon/folder.svg"
                        color: global.theme.general.fore
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }
                    Label {
                        text: qsTr("Root")
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    for (let i = 0; i < treeView.rows; ++i) {
                        treeView.collapseRecursively(i)
                    }
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Collapse All")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    for (let i = 0; i < treeView.rows; ++i) {
                        treeView.expandRecursively(i)
                    }
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Expand All")
                    }
                }
            }
        }

        TreeView {
            id: treeView
            clip: true
            acceptedButtons: Qt.NoButton
            model: root.explorerModule.model
            reuseItems: false
            rootIndex: root.modelRootIndex
            Layout.fillWidth: true; Layout.fillHeight: true
            property int selectedRow: -1

            function nodeAt(position) {
                const cell = cellAtPosition(contentItem.mapFromItem(treeView, position))
                return cell.y >= 0 ? itemAtCell(cell) : null
            }

            ScrollBar.vertical: ScrollBar {
                id: treeScrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: Item {
                id: node
                implicitWidth: treeView.width; implicitHeight: 24
                required property var model
                required property bool expanded
                required property int depth
                required property int row
                readonly property int gitStatus: {
                    const git = node.model.git
                    if (!git) return GitFileStatus.Unmodified
                    switch (git.indexStatus) {
                        case GitFileStatus.Untracked:
                        case GitFileStatus.Ignored:
                        case GitFileStatus.Added:
                        case GitFileStatus.Renamed:
                            return git.indexStatus
                    }
                    if (git.workingTreeStatus === GitFileStatus.Modified || git.workingTreeStatus === GitFileStatus.Unmerged)
                        return git.workingTreeStatus
                    return GitFileStatus.Unmodified
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.theme.general.backHover
                    opacity: hoverHandler.hovered && !fileDrag.active ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: AnimationDuration.Fade
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: treeView.selectedRow === node.row || dropArea.directory === node.model.documentUrl.toString()
                        ? global.theme.general.backSelected : "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.rightMargin: treeScrollBar.width
                    spacing: 0

                    Item {
                        Layout.preferredWidth: node.depth * 24; Layout.preferredHeight: 24
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: node.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                            color: global.theme.general.fore
                            visible: node.model.isDir === true
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            gesturePolicy: TapHandler.DragWithinBounds

                            onTapped: {
                                treeView.selectedRow = node.row
                                if (node.model.isDir) {
                                    treeView.toggleExpanded(node.row)
                                }
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        Image {
                            anchors.centerIn: parent
                            width: 20; height: 20
                            source: !node.expanded ? node.model.source
                                : node.model.source == "qrc:/icon/fileTypeFolder.svg" ? "qrc:/icon/fileTypeFolderOpen.svg"
                                    : node.model.source == "qrc:/icon/fileTypeFolderGit.svg" ? "qrc:/icon/fileTypeFolderOpenGit.svg"
                                        : node.model.source == "qrc:/icon/fileTypeFolderIntellij.svg" ? "qrc:/icon/fileTypeFolderOpenIntellij.svg"
                                            : node.model.source
                        }
                    }

                    Label {
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        color: GitFileStatusStyle.color(node.gitStatus)
                        text: node.model.display || ""
                        elide: Text.ElideRight
                        Layout.fillWidth: true; Layout.preferredHeight: 24
                    }
                }

                HoverHandler {
                    id: hoverHandler
                    enabled: !fileDrag.active

                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = node.gitStatus === GitFileStatus.Unmodified ? ""
                            : Qt.enumValueToString(GitFileStatus.Value, node.gitStatus)
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: treeView.selectedRow = node.row
                    onDoubleTapped: {
                        if (!node.model.isDir) {
                            root.explorerModule.documentOpen(node.model.documentUrl)
                        }
                    }
                }
            }

            DragHandler {
                id: fileDrag
                parent: treeView
                target: null
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                acceptedButtons: Qt.LeftButton
                property string sourceUrl
                property string name
                property url icon

                onActiveChanged: {
                    if (!active) return
                    const node = treeView.nodeAt(centroid.pressPosition)
                    if (!node) return
                    sourceUrl = node.model.documentUrl.toString()
                    name = node.model.display
                    icon = node.model.source
                    treeView.selectedRow = node.row
                    mainTooltip.text = ""
                    dragPreview.ensurePolished()
                    dragPreview.grabToImage(result => {
                        if (!fileDrag.active) return
                        dragPreview.Drag.imageSource = result.url
                        dragPreview.Drag.active = true
                    })
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton

                onTapped: treeView.selectedRow = -1
            }

            TapHandler {
                acceptedButtons: Qt.RightButton

                onTapped: {
                    const position = parent.mapToGlobal(point.position)
                    const node = treeView.nodeAt(treeView.mapFromItem(parent, point.position))
                    const localPosition = mainOverlay.mapFromGlobal(position)
                    if (!node) {
                        rootMenu.popup(localPosition.x, localPosition.y)
                    } else if (node.model.isDir) {
                        folderMenu.documentUrl = node.model.documentUrl
                        folderMenu.popup(localPosition.x, localPosition.y)
                    } else {
                        fileMenu.documentUrl = node.model.documentUrl
                        fileMenu.gitUntracked = node.model.git?.indexStatus === GitFileStatus.Untracked
                        fileMenu.gitIgnored = node.model.git?.indexStatus === GitFileStatus.Ignored
                        fileMenu.popup(localPosition.x, localPosition.y)
                    }
                }
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        keys: ["text/uri-list"]
        readonly property string directory: containsDrag ? directoryAt(drag.x, drag.y, drag.source) : ""

        function directoryAt(x, y, source) {
            let directory = root.modelRootUrl.toString()
            if (!rootTarget.contains(rootTarget.mapFromItem(dropArea, x, y))) {
                const position = treeView.mapFromItem(dropArea, x, y)
                if (!treeView.contains(position) || position.x >= treeView.width - treeScrollBar.width) return ""
                const node = treeView.nodeAt(position)
                if (node && !node.model.isDir) return ""
                if (node) directory = node.model.documentUrl.toString()
            }
            if (source === dragPreview) {
                const url = fileDrag.sourceUrl
                const prefix = directory.endsWith("/") ? directory : directory + "/"
                if (directory === url || directory.startsWith(url + "/") || prefix === url.slice(0, url.lastIndexOf("/") + 1)) return ""
            }
            return directory
        }

        onEntered: event => {
            if ((event.supportedActions & Qt.CopyAction) && event.urls.length
                && event.urls.every(url => url.toString().startsWith("file:"))) event.accept(Qt.CopyAction)
            else event.accepted = false
        }
        onPositionChanged: event => {
            if (directory && (event.supportedActions & Qt.CopyAction)) event.accept(Qt.CopyAction)
            else event.accepted = false
        }
        onDropped: event => {
            const target = directoryAt(event.x, event.y, event.source)
            if (!target || !(event.supportedActions & Qt.CopyAction)) {
                event.accepted = false
            } else if (event.source === dragPreview) {
                const prefix = target.endsWith("/") ? target : target + "/"
                if (global.fileModule.fileRename(fileDrag.sourceUrl, prefix + encodeURIComponent(fileDrag.name))) {
                    treeView.selectedRow = -1
                    event.accept(Qt.CopyAction)
                } else event.accepted = false
            } else {
                global.fileModule.fileCopy(event.urls, target)
                event.accept(Qt.CopyAction)
            }
        }
    }

    RowLayout {
        id: dragPreview
        x: -width
        Drag.dragType: Drag.Automatic
        Drag.source: dragPreview
        Drag.mimeData: ({ "text/uri-list": [fileDrag.sourceUrl] })
        Drag.supportedActions: Qt.CopyAction
        Drag.onDragFinished: fileDrag.sourceUrl = ""
        spacing: 4
        Image {
            Layout.preferredWidth: 16; Layout.preferredHeight: 16
            source: fileDrag.icon
        }
        Label { text: fileDrag.name }
    }

    Dialog {
        id: fileNewDialog
        parent: mainOverlay
        popupType: Popup.Item
        x: mainOverlay.mainGeometry.x + (mainOverlay.mainGeometry.width - width) / 2
        y: mainOverlay.mainGeometry.y + (mainOverlay.mainGeometry.height - height) / 2
        width: 600
        modal: true
        title: qsTr("New File")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            fileNameTextField.clear()
            fileNameTextField.forceActiveFocus()
        }
        onAccepted: {
            if (!fileNameTextField.acceptableInput) {
                return
            }
            mainOverlay.operationErrorShow(global.documentModule.documentCreate(fileNewDialog.documentUrl + "/" + fileNameTextField.text))
        }

        Binding {
            target: fileNewDialog.visible
                ? fileNewDialog.standardButton(Dialog.Ok) : null
            property: "enabled"
            value: fileNameTextField.acceptableInput
        }

        TextField {
            id: fileNameTextField
            width: parent.width
            placeholderText: qsTr("Enter file name:")
            validator: RegularExpressionValidator {
                regularExpression: /.*\..*/
            }

            onAccepted: fileNewDialog.accept()
            Keys.onEscapePressed: fileNewDialog.reject()
        }
    }

    Dialog {
        id: folderNewDialog
        parent: mainOverlay
        popupType: Popup.Item
        x: mainOverlay.mainGeometry.x + (mainOverlay.mainGeometry.width - width) / 2
        y: mainOverlay.mainGeometry.y + (mainOverlay.mainGeometry.height - height) / 2
        width: 600
        modal: true
        title: qsTr("New Folder")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            folderNameTextField.clear()
            folderNameTextField.forceActiveFocus()
        }
        onAccepted: mainOverlay.operationErrorShow(
            global.documentModule.directoryCreate(folderNewDialog.documentUrl + "/" + folderNameTextField.text))

        TextField {
            id: folderNameTextField
            width: parent.width
            placeholderText: qsTr("Enter folder name:")

            onAccepted: folderNewDialog.accept()
            Keys.onEscapePressed: folderNewDialog.reject()
        }
    }

    Menu {
        id: fileMenu
        parent: mainOverlay
        popupType: Popup.Item
        property bool gitUntracked
        property bool gitIgnored
        property string documentUrl

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Run")
            icon.source: "qrc:/icon/play.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.explorerModule.scriptRun(fileMenu.documentUrl)
        }

        MenuItem {
            text: qsTr("Debug")
            icon.source: "qrc:/icon/bug.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.explorerModule.scriptDebug(fileMenu.documentUrl)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Open In")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Explorer")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.fileModule.fileOpenInExplorer(fileMenu.documentUrl)
            }

            MenuItem {
                text: qsTr("Application")
                icon.source: "qrc:/icon/apps.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.fileModule.fileOpenInApplication(fileMenu.documentUrl)
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.showRenameDialog(fileMenu.documentUrl)
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    mainOverlay.operationErrorShow(global.documentModule.documentDelete(fileMenu.documentUrl))
                    progress = 0
                    fileMenu.close()
                }
            }
        }

        MenuItem {
            text: qsTr("Property")
            icon.source: "qrc:/icon/property.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.showPropertyDialog(fileMenu.documentUrl)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.serviceModule.git.hasRepository
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add")
                enabled: fileMenu.gitUntracked
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    global.gitModule.gitAdd(fileMenu.documentUrl)
                }
            }

            Menu {
                title: qsTr("Restore")
                enabled: !fileMenu.gitUntracked && !fileMenu.gitIgnored
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: global.gitModule.gitRestore(fileMenu.documentUrl, GitRestoreMode.Worktree)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: global.gitModule.gitRestore(fileMenu.documentUrl, GitRestoreMode.Staged)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: global.gitModule.gitRestore(fileMenu.documentUrl, GitRestoreMode.Both)
                }
            }

            MenuItem {
                text: fileMenu.gitIgnored ? qsTr("Unignore") : qsTr("Ignore")
                icon.source: "qrc:/icon/prohibited.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.gitModule.gitIgnore(fileMenu.documentUrl, !fileMenu.gitIgnored)
            }
        }
    }

    Menu {
        id: folderMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string documentUrl

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Open In Explorer")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.fileOpenInExplorer(folderMenu.documentUrl)
        }

        Menu {
            title: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("File")
                icon.source: "qrc:/icon/document.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    fileNewDialog.documentUrl = folderMenu.documentUrl
                    fileNewDialog.open()
                }
            }

            MenuItem {
                text: qsTr("Folder")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    folderNewDialog.documentUrl = folderMenu.documentUrl
                    folderNewDialog.open()
                }
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.showRenameDialog(folderMenu.documentUrl)
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    mainOverlay.operationErrorShow(global.documentModule.directoryDelete(folderMenu.documentUrl))
                    progress = 0
                    folderMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.serviceModule.git.hasRepository
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add Folder")
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    global.gitModule.gitAdd(folderMenu.documentUrl)
                }
            }

            Menu {
                title: qsTr("Restore Folder")
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: global.gitModule.gitRestore(folderMenu.documentUrl, GitRestoreMode.Worktree)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: global.gitModule.gitRestore(folderMenu.documentUrl, GitRestoreMode.Staged)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: global.gitModule.gitRestore(folderMenu.documentUrl, GitRestoreMode.Both)
                }
            }

            MenuItem {
                text: qsTr("Ignore")
                icon.source: "qrc:/icon/prohibited.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.gitModule.gitIgnore(folderMenu.documentUrl, true)
            }
        }
    }

    Menu {
        id: rootMenu
        parent: mainOverlay
        popupType: Popup.Item
        readonly property url documentUrl: root.modelRootUrl

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Open In Explorer")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.fileOpenInExplorer(rootMenu.documentUrl)
        }

        Menu {
            title: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("File")
                icon.source: "qrc:/icon/document.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    fileNewDialog.documentUrl = rootMenu.documentUrl
                    fileNewDialog.open()
                }
            }

            MenuItem {
                text: qsTr("Folder")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    folderNewDialog.documentUrl = rootMenu.documentUrl
                    folderNewDialog.open()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.serviceModule.git.hasRepository
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add All")
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    global.gitModule.gitAdd()
                }
            }

            Menu {
                title: qsTr("Restore All")
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: global.gitModule.gitRestore("", GitRestoreMode.Worktree)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: global.gitModule.gitRestore("", GitRestoreMode.Staged)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: global.gitModule.gitRestore("", GitRestoreMode.Both)
                }
            }
        }
    }
}

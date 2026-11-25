import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import "../../util"

import UniComm

Item {
    id: root
    required property GitModule gitModule
    readonly property bool hasRepository: global.serviceModule.git.hasRepository
    property bool hasRefs: root.gitModule.refs.rowCount() > 0
    anchors.fill: parent

    function saveState() { return splitView.saveState() }
    function restoreState(state) { return splitView.restoreState(state) }

    SortFilterProxyModel {
        id: refFilter
        model: root.gitModule.refs
        recursiveFiltering: true
        filters: FunctionFilter {
            column: 0
            component RoleData: QtObject {
                property string ref
            }

            function filter(data: RoleData): bool {
                return !searchField.query.length || data.ref.toLowerCase().includes(searchField.query)
            }
        }
    }

    SortFilterProxyModel {
        id: logFilter
        model: root.gitModule.log
        filters: FunctionFilter {
            column: 1
            component LogData: QtObject {
                property string display
                property string hash
            }

            function filter(data: LogData): bool {
                return data.display.toLowerCase().includes(logSearchField.query)
                    || data.hash.startsWith(logSearchField.query)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    Item {
        anchors.fill: parent
        visible: global.serviceModule.git.status !== 1 && (!root.hasRepository || !root.hasRefs)

        RowLayout {
            anchors.centerIn: parent

            Button {
                flat: true
                enabled: global.serviceModule.git.status !== 2
                text: !global.serviceModule.git.available ? qsTr("Git is unavailable.")
                    : !enabled ? qsTr("Unable to open Git repository.")
                    : !root.hasRepository ? qsTr("Click to create git repository.") : qsTr("Click to create first commit.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter

                onClicked: !root.hasRepository ? root.gitModule.gitInit() : root.gitModule.gitCommitPre()
            }

            IconImage {
                source: !root.hasRepository ? "qrc:/icon/fileTypeGit.svg" : "qrc:/icon/gitCommit.svg"
                color: global.theme.general.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal
        visible: root.hasRepository && root.hasRefs
        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: global.theme.general.stroke
            }
        }

        RowLayout {
            SplitView.preferredWidth: 400; SplitView.fillHeight: true

            ColumnLayout {
                Layout.minimumWidth: 24; Layout.maximumWidth: 24
                Layout.alignment: Qt.AlignTop

                Button {
                    flat: true
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    icon.source: "qrc:/icon/addSquare.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Accessible.name: qsTr("Create Branch")

                    onClicked: {
                        const row = branchTreeView.selectedRow
                        const index = branchTreeView.index(row, 0)
                        const ref = row >= 0 ? refFilter.data(index, GitModule.RefRole) : ""
                        branchCreateDialog.openFor(ref || "HEAD", ref ? refFilter.data(index, Qt.DisplayRole) : "")
                    }

                    HoverHandler {
                        onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = parent.Accessible.name
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true

                LineEdit {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Branch or tag")
                    clearable: true
                    readonly property string query: text.trim().toLowerCase()

                    onQueryChanged: {
                        refFilter.invalidate()
                        branchTreeView.selectedRow = -1
                        branchTreeView.contentY = 0
                        Qt.callLater(() => branchTreeView.expandRecursively())
                    }
                }

                TreeView {
                    id: branchTreeView
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true
                    model: refFilter
                    reuseItems: false
                    property int selectedRow: -1

                    ScrollBar.vertical: ScrollBar {
                        id: treeScrollBar
                        minimumSize: Math.min(1, 20 / Math.max(1, height))
                        palette {
                            mid: global.theme.general.stroke
                            dark: global.theme.general.strokePressed
                        }
                    }

                    delegate: Item {
                        implicitWidth: branchTreeView.width; implicitHeight: 24
                        required property var model
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
                                    duration: AnimationDuration.Fade
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: branchTreeView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.rightMargin: treeScrollBar.width
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
                                    enabled: isTreeNode && hasChildren
                                    acceptedButtons: Qt.LeftButton
                                    gesturePolicy: TapHandler.DragWithinBounds

                                    onTapped: {
                                        branchTreeView.selectedRow = row
                                        branchTreeView.toggleExpanded(row)
                                    }
                                }
                            }

                            Item {
                                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                IconImage {
                                    anchors.centerIn: parent
                                    width: 16; height: 16
                                    source: GitStyle.refIcon(model.type)
                                    color: GitStyle.refColor(model.type)
                                }
                            }

                            Label {
                                horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                text: model.display
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                Layout.preferredHeight: 24
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
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = model.ref || ""
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            gesturePolicy: TapHandler.DragWithinBounds

                            onTapped: branchTreeView.selectedRow = row
                            onDoubleTapped: {
                                if (!hasChildren) root.gitModule.logGet(model.ref)
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            gesturePolicy: TapHandler.DragWithinBounds

                            onTapped: {
                                if (!hasChildren && model.type !== GitRefType.Tag) {
                                    branchMenu.ref = model.ref
                                    branchMenu.name = model.display
                                    branchMenu.type = model.type
                                    const position = mainOverlay.mapFromGlobal(parent.mapToGlobal(point.position))
                                    branchMenu.popup(position.x, position.y)
                                }
                            }
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton

                        onTapped: branchTreeView.selectedRow = -1
                    }
                }
            }
        }

        ColumnLayout {
            SplitView.fillWidth: true; SplitView.fillHeight: true

            LineEdit {
                id: logSearchField
                Layout.fillWidth: true
                placeholderText: qsTr("Text or hash")
                clearable: true
                readonly property string query: text.trim().toLowerCase()

                onQueryChanged: {
                    logFilter.invalidate()
                    tableView.selectedRow = -1
                    tableView.hoveredRow = -1
                    tableView.contentY = 0
                    canvas.requestPaint()
                }
            }

            TableView {
                id: tableView
                alternatingRows: false
                clip: true
                editTriggers: TableView.NoEditTriggers
                model: logFilter
                contentWidth: width
                Layout.fillWidth: true; Layout.fillHeight: true
                property int hoveredRow: -1
                property int selectedRow: -1

                ScrollBar.vertical: ScrollBar {
                    id: logScrollBar
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                columnWidthProvider: function (column: int): real {
                    if (column === 0) return canvas.width
                    const available = Math.max(0, Math.floor(width - canvas.width - logScrollBar.width))
                    const author = Math.max(60, implicitColumnWidth(2))
                    const date = Math.max(60, implicitColumnWidth(3))
                    const scale = Math.min(1, available / (date + author))
                    if (column === 2) return Math.floor(author * scale)
                    if (column === 3) return Math.floor(date * scale) + logScrollBar.width
                    return available - Math.floor(date * scale) - Math.floor(author * scale)
                }
                onWidthChanged: forceLayout()

                delegate: Item {
                    implicitWidth: Math.max(textMetrics.width + 16, 60)
                    implicitHeight: 24
                    required property var model
                    required property int column
                    required property int row

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.general.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        topLeftRadius: column === 0 ? radius : 0
                        bottomLeftRadius: column === 0 ? radius : 0
                        topRightRadius: column === tableView.columns - 1 ? radius : 0
                        bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                        color: global.theme.general.backHover
                        opacity: tableView.hoveredRow === row ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: AnimationDuration.Fade
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        topLeftRadius: column === 0 ? radius : 0
                        bottomLeftRadius: column === 0 ? radius : 0
                        topRightRadius: column === tableView.columns - 1 ? radius : 0
                        bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                        color: tableView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                    }

                    TextMetrics {
                        id: textMetrics
                        font: label.font
                        text: model.display || ""
                    }

                    Label {
                        id: label
                        anchors.fill: parent
                        anchors.rightMargin: column === 3 ? logScrollBar.width : 0
                        leftPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display || ""
                        elide: Text.ElideRight
                        visible: column !== 0
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

                        onTapped: {
                            tableView.selectedRow = row
                            root.gitModule.showGet(model.hash)
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onTapped: {
                            logMenu.hash = model.hash
                            const position = mainOverlay.mapFromGlobal(parent.mapToGlobal(point.position))
                            logMenu.popup(position.x, position.y)
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onTapped: tableView.selectedRow = -1
                }

                Canvas {
                    id: canvas
                    parent: tableView.contentItem

                    width: 24 * root.gitModule.laneCount
                    height: tableView.contentHeight
                    z: 100

                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()

                        const unit = 24
                        const colors = ["#264653", "#2a9d8f", "#e9c46a", "#f4a261", "#e76f51"]

                        for (let i = 0; i < tableView.model.rowCount(); ++i) {
                            const index = tableView.model.index(i, 0)
                            const pos = tableView.model.data(index, GitModule.PositionRole)
                            const parents = tableView.model.data(index, GitModule.ParentsRole)
                            const _x = pos.x * unit + unit / 2
                            const _y = i * unit + unit / 2

                            for (let j = 0; j < parents.length; ++j) {
                                const parentPos = parents[j]
                                const parentRow = logFilter.mapFromSource(root.gitModule.log.index(parentPos.y, 0)).row
                                if (parentRow < 0) continue
                                const px = parentPos.x * unit + unit / 2
                                const py = parentRow * unit + unit / 2
                                const dy = py - _y
                                ctx.lineWidth = 2
                                ctx.strokeStyle = colors[Math.max(pos.x, parentPos.x) % colors.length]
                                ctx.beginPath()
                                ctx.moveTo(_x, _y)

                                const dx = px - _x
                                const diagonalHeight = Math.abs(dx)
                                if (dx === 0) {
                                    ctx.lineTo(px, py)
                                } else if (diagonalHeight <= dy) {
                                    if (dx > 0) {
                                        ctx.lineTo(px, _y + diagonalHeight)
                                        ctx.lineTo(px, py)
                                    } else {
                                        ctx.lineTo(_x, py - diagonalHeight)
                                        ctx.lineTo(px, py)
                                    }
                                } else {
                                    ctx.lineTo(px, py)
                                }
                                ctx.stroke()
                            }
                        }

                        for (let i = 0; i < tableView.model.rowCount(); ++i) {
                            const index = tableView.model.index(i, 0)
                            const pos = tableView.model.data(index, GitModule.PositionRole)
                            const _x = pos.x * unit + unit / 2
                            const _y = i * unit + unit / 2
                            ctx.fillStyle = colors[pos.x % colors.length]
                            ctx.beginPath()
                            ctx.arc(_x, _y, 4, 0, Math.PI * 2)
                            ctx.fill()
                        }
                    }
                }
            }
        }

        ColumnLayout {
            SplitView.preferredWidth: 400

            RowLayout {
                id: showToolBar
                Layout.fillWidth: true
                spacing: 0

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/collapse.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: showTreeView.collapseRecursively()

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

                    onClicked: showTreeView.expandRecursively()

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
                id: showTreeView
                width: parent.width
                clip: true
                model: root.gitModule.show
                Layout.fillWidth: true; Layout.fillHeight: true
                property int selectedRow: -1

                ScrollBar.vertical: ScrollBar {
                    id: showScrollBar
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: Item {
                    implicitWidth: showTreeView.width; implicitHeight: 24
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
                        color: showTreeView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.rightMargin: showScrollBar.width
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
                                    showTreeView.selectedRow = row
                                    if (isTreeNode && hasChildren) {
                                        showTreeView.toggleExpanded(row)
                                    }
                                }
                            }
                        }

                        Item {
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24

                            IconImage {
                                anchors.centerIn: parent
                                width: 16; height: 16
                                source: !expanded ? model.decoration
                                    : model.decoration == "qrc:/icon/fileTypeFolder.svg" ? "qrc:/icon/fileTypeFolderOpen.svg"
                                        : model.decoration
                            }
                        }

                        Label {
                            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                            text: model.display || ""
                            elide: Text.ElideRight
                            color: GitStyle.fileStatusColor(model.status)
                            Layout.preferredHeight: 24; Layout.fillWidth: true
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

                        onTapped: showTreeView.selectedRow = row
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onTapped: {
                            if (!(isTreeNode && hasChildren)) {
                                root.gitModule.gitShowFile(model.documentUrl)
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onTapped: showTreeView.selectedRow = -1
                }
            }

            ColumnLayout {
                implicitHeight: subjectLabel.implicitHeight + dateLabel.implicitHeight + authorLabel.implicitHeight + spacing * 2

                Label {
                    id: subjectLabel
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    id: dateLabel
                    horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    id: authorLabel
                    horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }

    Connections {
        target: root.gitModule

        function onExpandBranches() {
            root.hasRefs = root.gitModule.refs.rowCount() > 0
            branchTreeView.selectedRow = -1
            branchTreeView.expandRecursively()
        }
        function onLogChanged() {
            tableView.selectedRow = logFilter.mapFromSource(root.gitModule.log.index(0, 0)).row
            tableView.contentY = 0
            tableView.forceLayout()
            canvas.requestPaint()
        }
        function onSetCommitDetails(subject, date, author) {
            subjectLabel.text = subject
            dateLabel.text = date
            authorLabel.text = author
        }
        function onShowContinueDialog() { continueDialog.open() }
        function onShowRemoteAddDialog() { remoteAddDialog.open() }
        function onShowErrorDialog(title, text) {
            errorDialog.title = title
            errorDialog.text = text
            errorDialog.open()
        }
        function onShowProxyDialog(proxies) {
            localHttpProxyTextField.text = proxies.localHttpProxy
            localHttpsProxyTextField.text = proxies.localHttpsProxy
            globalHttpProxyTextField.text = proxies.globalHttpProxy
            globalHttpsProxyTextField.text = proxies.globalHttpsProxy
            proxyDialog.open()
        }
    }

    Dialog {
        id: continueDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        title: root.gitModule.gitStatus === GitOperationState.Merge ? qsTr("Finish Merge") : qsTr("Finish Rebase")
        standardButtons: Dialog.Ok

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            continueCheckBox.checked = root.gitModule.gitStatus !== GitOperationState.Merge
            continueCheckBox.enabled = root.gitModule.gitStatus === GitOperationState.Merge
            continueTextField.text = ""
            continueTextField.forceActiveFocus()
        }

        onAccepted: root.gitModule.gitContinue(continueTextField.text)

        ColumnLayout {
            width: parent.width

            CheckBox {
                id: continueCheckBox
                text: qsTr("Used auto-generated message")
            }

            TextField {
                id: continueTextField
                visible: !continueCheckBox.checked
                placeholderText: qsTr("Enter commit message:")
                Layout.fillWidth: true

                onAccepted: continueDialog.accept()
                Keys.onEscapePressed: continueDialog.reject()
            }
        }
    }

    Dialog {
        id: errorDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        standardButtons: Dialog.Ok
        property string text

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        Label {
            width: parent.width
            text: errorDialog.text
            wrapMode: Text.WrapAnywhere
        }
    }

    Dialog {
        id: branchCreateDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Create branch from %1").arg(displayName || startPoint)
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string startPoint
        property string displayName

        function openFor(ref, displayName) {
            startPoint = ref
            branchCreateDialog.displayName = displayName
            open()
        }

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            branchCreateTextField.text = branchCreateDialog.displayName
            branchCreateTextField.forceActiveFocus()
            branchCreateTextField.selectAll()
            branchSwitchCheckBox.checked = true
        }

        onAccepted: root.gitModule.branchCreate(startPoint, branchCreateTextField.text, branchSwitchCheckBox.checked)

        ColumnLayout {
            width: parent.width

            TextField {
                id: branchCreateTextField
                placeholderText: qsTr("Enter branch name:")
                Layout.fillWidth: true

                onAccepted: branchCreateDialog.accept()
                Keys.onEscapePressed: branchCreateDialog.reject()
            }

            RowLayout {
                Layout.preferredHeight: 32

                CheckBox {
                    id: branchSwitchCheckBox
                    text: qsTr("Switch")
                }
            }
        }
    }

    Dialog {
        id: proxyDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Proxy Configuration")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        onAccepted: root.gitModule.gitProxySet(
            localHttpProxyTextField.text,
            localHttpsProxyTextField.text,
            globalHttpProxyTextField.text,
            globalHttpsProxyTextField.text)

        ColumnLayout {
            width: parent.width

            Label {
                text: qsTr("Local Http Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: localHttpProxyTextField
                Layout.fillWidth: true

                onAccepted: proxyDialog.accept()
                Keys.onEscapePressed: proxyDialog.reject()
            }

            Label {
                text: qsTr("Local Https Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: localHttpsProxyTextField
                Layout.fillWidth: true

                onAccepted: proxyDialog.accept()
                Keys.onEscapePressed: proxyDialog.reject()
            }

            Label {
                text: qsTr("Global Http Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: globalHttpProxyTextField
                Layout.fillWidth: true

                onAccepted: proxyDialog.accept()
                Keys.onEscapePressed: proxyDialog.reject()
            }

            Label {
                text: qsTr("Global Https Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: globalHttpsProxyTextField
                Layout.fillWidth: true

                onAccepted: proxyDialog.accept()
                Keys.onEscapePressed: proxyDialog.reject()
            }
        }
    }

    Dialog {
        id: branchRenameDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Rename branch ") + branchRenameDialog.name
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string name

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            branchRenameTextField.text = branchRenameDialog.name
            branchRenameTextField.forceActiveFocus()
            branchRenameTextField.selectAll()
        }

        onAccepted: root.gitModule.gitRename(branchRenameDialog.name, branchRenameTextField.text)

        TextField {
            id: branchRenameTextField
            placeholderText: qsTr("Enter branch name:")
            width: parent.width

            onAccepted: branchRenameDialog.accept()
            Keys.onEscapePressed: branchRenameDialog.reject()
        }
    }

    Dialog {
        id: remoteAddDialog
        parent: mainScreen
        popupType: Popup.Item
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Add Remote Repository")
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            remoteAddTextField.text = ""
            remoteAddTextField.forceActiveFocus()
        }

        onAccepted: root.gitModule.gitRemoteAdd(remoteAddTextField.text)

        TextField {
            id: remoteAddTextField
            placeholderText: qsTr("Remote URL (e.g., https://github.com/user/repo.git)")
            width: parent.width

            onAccepted: remoteAddDialog.accept()
            Keys.onEscapePressed: remoteAddDialog.reject()
        }
    }

    Menu {
        id: branchMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string ref
        property string name
        property int type
        readonly property bool local: type === GitRefType.Local || type === GitRefType.Current

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Switch")
            enabled: branchMenu.type === GitRefType.Local
            icon.source: "qrc:/icon/arrowRight.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.gitModule.gitSwitch(branchMenu.name)
        }

        MenuItem {
            text: qsTr("Create")
            enabled: branchMenu.local
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: branchCreateDialog.openFor(branchMenu.ref, branchMenu.name)
        }

        MenuItem {
            text: qsTr("Rename")
            enabled: branchMenu.local
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                branchRenameDialog.name = branchMenu.name
                branchRenameDialog.open()
            }
        }

        Menu {
            title: qsTr("Delete")
            enabled: branchMenu.local
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    root.gitModule.gitDelete(branchMenu.name)
                    progress = 0
                    branchMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        MenuItem {
            text: branchMenu.type === GitRefType.Upstream ? qsTr("Unset Upstream") : qsTr("Set Upstream")
            enabled: root.gitModule.gitStatus === GitOperationState.Idle && (branchMenu.type === GitRefType.Upstream || branchMenu.type === GitRefType.Remote)
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            onTriggered: branchMenu.type === GitRefType.Upstream ? root.gitModule.gitUpstreamUnset() : root.gitModule.gitUpstreamSet(branchMenu.name)
        }

        MenuItem {
            text: qsTr("Merge")
            enabled: root.gitModule.gitStatus === GitOperationState.Idle
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.gitModule.gitMerge(branchMenu.name)
        }

        MenuItem {
            text: qsTr("Rebase")
            enabled: root.gitModule.gitStatus === GitOperationState.Idle
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.gitModule.gitRebase(branchMenu.name)
        }
    }

    Menu {
        id: logMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string hash

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Cherry-pick")
            enabled: root.gitModule.gitStatus === GitOperationState.Idle
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: root.gitModule.gitCherryPick(logMenu.hash)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Reset")
            icon.source: "qrc:/icon/reset.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Mixed")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Leave your working directory unchanged.")
                    }
                }

                onTriggered: root.gitModule.gitReset(logMenu.hash, GitResetMode.Mixed)
            }

            MenuItem {
                text: qsTr("Soft")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Leave your working tree files and the index unchanged.")
                    }
                }

                onTriggered: root.gitModule.gitReset(logMenu.hash, GitResetMode.Soft)
            }

            MenuItem {
                text: qsTr("Hard")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Overwrite all files and directories with the version from commit, and may overwrite untracked files.")
                    }
                }

                onTriggered: root.gitModule.gitReset(logMenu.hash, GitResetMode.Hard)
            }

            MenuItem {
                text: qsTr("Merge")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Reset the index and update the files in the working tree that are different between commit and HEAD, but keep those which are different between the index and working tree.")
                    }
                }

                onTriggered: root.gitModule.gitReset(logMenu.hash, GitResetMode.Merge)
            }

            MenuItem {
                text: qsTr("Keep")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Resets index entries and updates files in the working tree that are different between commit and HEAD.")
                    }
                }

                onTriggered: root.gitModule.gitReset(logMenu.hash, GitResetMode.Keep)
            }
        }
    }
}

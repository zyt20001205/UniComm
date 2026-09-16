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

    Item {
        anchors.fill: parent
        visible: branchModel.empty

        RowLayout {
            anchors.centerIn: parent

            Button {
                flat: true
                text: !global.gitEnabled ? qsTr("Click to create git repository.") : qsTr("Click to create first commit.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter

                onClicked: !global.gitEnabled ? gitModule.gitInit() : gitModule.gitCommitPre()
            }

            IconImage {
                source: !global.gitEnabled ? "qrc:/icon/fileTypeGit.svg" : "qrc:/icon/gitCommit.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        visible: global.gitEnabled && !branchModel.empty
        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: global.stroke
            }
        }

        TreeView {
            id: branchTreeView
            SplitView.preferredWidth: 400; SplitView.fillHeight: true
            clip: true
            model: branchModel
            property int selectedRow: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            delegate: Item {
                implicitWidth: branchTreeView.width; implicitHeight: 24
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
                    color: branchTreeView.selectedRow === row ? global.backSelected : "transparent"
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
                                branchTreeView.selectedRow = row
                                if (isTreeNode && hasChildren) {
                                    branchTreeView.toggleExpanded(row)
                                }
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: model.type === "localRep" ? "qrc:/icon/tcpClient.svg"
                                : model.type === "remoteRep" ? "qrc:/icon/tcpServer.svg"
                                    : model.type === "current" ? "qrc:/icon/tag.svg"
                                        : model.type === "upstream" ? "qrc:/icon/link.svg"
                                            : "qrc:/icon/gitBranch.svg"
                            color: ["current", "upstream"].includes(model.type) ? global.warningFore3 : global.fore
                        }
                    }

                    Label {
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display
                        elide: Text.ElideRight
                        Layout.preferredHeight: 24
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                        text: model.commit || ""
                        elide: Text.ElideLeft
                        Layout.preferredHeight: 24
                        Layout.rightMargin: 10
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
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = model.hash || ""
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        branchTreeView.selectedRow = row
                        if (!(isTreeNode && hasChildren)) {
                            {
                                gitModule.branchSet(model.display)
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        if (!(isTreeNode && hasChildren)) {
                            branchMenu.name = model.display
                            branchMenu.type = model.type
                            branchMenu.popup()
                        }
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton

                onTapped: branchTreeView.selectedRow = -1
            }
        }

        TableView {
            id: tableView
            alternatingRows: false
            clip: true
            editTriggers: TableView.NoEditTriggers
            model: logModel
            contentWidth: width
            SplitView.fillWidth: true
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

            columnWidthProvider: function (column) {
                if (column !== columns - 1) return implicitColumnWidth(column)
                let usedWidth = 0
                for (let i = 0; i < columns - 2; ++i) usedWidth += implicitColumnWidth(i)
                return width - usedWidth
            }

            delegate: Item {
                implicitWidth: Math.max(textMetrics.width + 16, 60)
                implicitHeight: 24
                required property int column
                required property int row

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    topLeftRadius: column === 0 ? radius : 0
                    bottomLeftRadius: column === 0 ? radius : 0
                    topRightRadius: column === tableView.columns - 1 ? radius : 0
                    bottomRightRadius: column === tableView.columns - 1 ? radius : 0
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
                    topLeftRadius: column === 0 ? radius : 0
                    bottomLeftRadius: column === 0 ? radius : 0
                    topRightRadius: column === tableView.columns - 1 ? radius : 0
                    bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                    color: tableView.selectedRow === row ? global.backSelected : "transparent"
                }

                TextMetrics {
                    id: textMetrics
                    font: label.font
                    text: model.display || ""
                }

                Label {
                    id: label
                    anchors.fill: parent
                    leftPadding: 6
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: model.display || ""
                    elide: Text.ElideRight
                    visible: column !== tableView.columns - 1
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (hovered) {
                            tableView.hoveredRow = row
                        } else if (tableView.hoveredRow === row) {
                            tableView.hoveredRow = -1
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = model.hash || ""
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        tableView.selectedRow = row
                        gitModule.gitShowCommit(model.hash)
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        logMenu.hash = model.hash
                        logMenu.popup()
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

                x: tableView.width - width
                y: 0
                width: 24 * laneCount
                height: tableView.contentHeight
                z: 100
                property int laneCount: 0

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()

                    const unit = 24
                    const colors = ["#264653", "#2a9d8f", "#e9c46a", "#f4a261", "#e76f51"]

                    for (let i = 0; i < logModel.rowCount(); ++i) {
                        const index = logModel.index(i, 3);
                        const pos = logModel.data(index, Qt.UserRole + 2)
                        const parents = logModel.data(index, Qt.UserRole + 3) || []
                        const _x = pos.x * unit + unit / 2
                        const _y = pos.y * unit + unit / 2

                        for (let j = 0; j < parents.length; ++j) {
                            const parentPos = parents[j]
                            const px = parentPos.x * unit + unit / 2
                            const py = parentPos.y * unit + unit / 2
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

                    for (let i = 0; i < logModel.rowCount(); ++i) {
                        const index = logModel.index(i, 3)
                        const pos = logModel.data(index, Qt.UserRole + 2)
                        const _x = pos.x * unit + unit / 2
                        const _y = pos.y * unit + unit / 2
                        ctx.fillStyle = colors[pos.x % colors.length]
                        ctx.beginPath()
                        ctx.arc(_x, _y, 4, 0, Math.PI * 2)
                        ctx.fill()
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
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Collapse All")
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
                                mainToolTip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = qsTr("Expand All")
                        }
                    }
                }
            }

            TreeView {
                id: showTreeView
                width: parent.width
                clip: true
                model: showModel
                Layout.fillWidth: true; Layout.fillHeight: true
                property int selectedRow: -1

                ScrollBar.vertical: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
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
                        color: showTreeView.selectedRow === row ? global.backSelected : "transparent"
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
                            color: {
                                switch (model.status) {
                                    case 3:
                                        return global.brandBack
                                    case 5:
                                        return global.successFore2
                                    case 6:
                                        return global.stroke
                                    case 7:
                                        return global.warningFore3
                                    default:
                                        return global.fore
                                }
                            }
                            Layout.preferredHeight: 24; Layout.fillWidth: true
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
                                    default:
                                        mainToolTip.text = "contact author: unsupported status (" + model.status + ")"
                                }
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
                                gitModule.gitShowFile(showModel.hash, model.documentUrl)
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
        target: logModel

        function onRowsInserted() {
            canvas.requestPaint()
        }

        function onRowsRemoved() {
            canvas.requestPaint()
        }

        function onModelReset() {
            canvas.requestPaint()
            tableView.selectedRow = 0
        }
    }

    function branchExpand() {
        branchTreeView.expandRecursively()
    }

    Component.onCompleted: {
        const objects = {
            "canvas": canvas,
            "subjectLabel": subjectLabel,
            "dateLabel": dateLabel,
            "authorLabel": authorLabel
        };
        gitModule.propertyGet(objects)
    }
}

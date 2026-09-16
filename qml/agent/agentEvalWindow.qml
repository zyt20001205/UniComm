pragma ComponentBehavior: Bound

import QtGraphs
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    readonly property var summary: evalModel.summary
    readonly property real selectedDuration: turnHistory.selectedNodeId === ""
        ? summary.durationTotal
        : turnHistory.selectedDuration
    readonly property real selectedToolDuration: turnHistory.selectedNodeId === ""
        ? summary.toolDuration
        : turnHistory.selectedToolDuration

    function statusColor(status: int): color {
        switch (status) {
            case 2:
                return global.successBack3
            case 3:
                return global.warningBack3
            case 4:
                return global.dangerBack3
            default:
                return global.brandBack
        }
    }

    function ttftUpdate(): void {
        ttftSeries.clear()
        for (let index = 0; index < summary.ttftValues.length; ++index) {
            ttftSeries.append(index + 1, summary.ttftValues[index])
        }
    }

    Component.onCompleted: ttftUpdate()

    GraphsTheme {
        id: graphsTheme
        theme: GraphsTheme.Theme.QtGreen
        backgroundVisible: false
        plotAreaBackgroundVisible: false
        gridVisible: false
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        rightPadding: effectiveScrollBarWidth
        bottomPadding: effectiveScrollBarHeight

        ScrollBar.vertical: ScrollBar {
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            x: scrollView.mirrored ? 0 : scrollView.width - width
            y: scrollView.topPadding
            height: scrollView.availableHeight
            active: scrollView.ScrollBar.horizontal.active
            policy: ScrollBar.AsNeeded
            palette {
                mid: global.stroke
                dark: global.strokePressed
            }
        }

        ScrollBar.horizontal: ScrollBar {
            minimumSize: Math.min(1, 20 / Math.max(1, width))
            x: scrollView.leftPadding
            y: scrollView.height - height
            width: scrollView.availableWidth
            active: scrollView.ScrollBar.vertical.active
            policy: ScrollBar.AsNeeded
            palette {
                mid: global.stroke
                dark: global.strokePressed
            }
        }

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.topMargin: 18; Layout.rightMargin: 20

                Label {
                    text: qsTr("Evaluation")
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }

                Label {
                    text: summary.runCount === 0
                        ? qsTr("Current conversation · No completed turns")
                        : qsTr("Current conversation · %1 / %2 · %3 turns")
                            .arg(summary.provider)
                            .arg(summary.model)
                            .arg(summary.runCount)
                    color: global.foreDisabled
                }
            }

            GridLayout {
                columns: 4
                columnSpacing: 10; rowSpacing: 10
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.rightMargin: 20

                StatCard {
                    title: qsTr("Runs")
                    value: summary.runCount.toString()
                    detail: qsTr("%1 completed · %2 aborted · %3 errors")
                        .arg(summary.completedCount)
                        .arg(summary.abortedCount)
                        .arg(summary.errorCount)
                    Layout.fillWidth: true
                }

                StatCard {
                    title: qsTr("Latency")
                    value: summary.durationAverage
                    detail: qsTr("TTFT %1 · Max %2")
                        .arg(summary.ttftAverage)
                        .arg(summary.durationMaximum)
                    Layout.fillWidth: true
                }

                StatCard {
                    title: qsTr("Tokens")
                    value: summary.totalTokens
                    detail: qsTr("%1 prompt · %2 output")
                        .arg(summary.promptTokens)
                        .arg(summary.completionTokens)
                    Layout.fillWidth: true
                }

                StatCard {
                    title: qsTr("Activity")
                    value: qsTr("%1 tools").arg(summary.toolCalls)
                    detail: qsTr("%1 model calls · %2 tool time")
                        .arg(summary.modelCalls)
                        .arg(summary.toolDurationText)
                    Layout.fillWidth: true
                }
            }

            GridLayout {
                columns: 2
                columnSpacing: 10; rowSpacing: 10
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.rightMargin: 20

                Panel {
                    title: qsTr("Time breakdown")
                    detail: turnHistory.selectedNodeId === ""
                        ? qsTr("All turns")
                        : turnHistory.selectedTitle
                    Layout.preferredWidth: 300; Layout.preferredHeight: 280

                    Item {
                        anchors.fill: parent

                        GraphsView {
                            id: timeGraph
                            anchors.fill: parent
                            marginLeft: 0; marginTop: 0; marginRight: 0; marginBottom: 32
                            theme: graphsTheme

                            PieSeries {
                                pieSize: 0.9
                                holeSize: 0.72

                                PieSlice {
                                    id: toolSlice
                                    value: rootItem.selectedToolDuration
                                    color: graphsTheme.seriesColors[0]
                                }

                                PieSlice {
                                    id: nonToolSlice
                                    value: Math.max(0, rootItem.selectedDuration - rootItem.selectedToolDuration)
                                    color: graphsTheme.seriesColors[1]
                                }
                            }
                        }

                        Label {
                            x: timeGraph.plotArea.x + (timeGraph.plotArea.width - width) / 2
                            y: timeGraph.plotArea.y + (timeGraph.plotArea.height - height) / 2
                            text: turnHistory.selectedNodeId === ""
                                ? summary.durationTotalText
                                : turnHistory.selectedDurationText
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        RowLayout {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            spacing: 16

                            LegendItem {
                                markerColor: graphsTheme.seriesColors[0]
                                text: qsTr("Tool %1%").arg((toolSlice.percentage * 100).toFixed(0))
                            }

                            LegendItem {
                                markerColor: graphsTheme.seriesColors[1]
                                text: qsTr("Non-tool %1%").arg((nonToolSlice.percentage * 100).toFixed(0))
                            }
                        }
                    }
                }

                Panel {
                    title: qsTr("Latency trend")
                    detail: qsTr("TTFT")
                    Layout.fillWidth: true; Layout.preferredHeight: 280

                    Item {
                        anchors.fill: parent

                        GraphsView {
                            anchors.fill: parent
                            axisX: ValueAxis {
                                min: summary.runCount === 1 ? 0 : 1
                                max: Math.max(2, summary.runCount)
                                tickInterval: 1
                                labelDecimals: 0
                            }
                            axisY: ValueAxis {
                                min: 0
                                max: Math.max(1, summary.ttftMaximumValue)
                            }
                            marginLeft: 0; marginTop: 0; marginRight: 0; marginBottom: 32
                            theme: graphsTheme

                            LineSeries {
                                id: ttftSeries
                                color: graphsTheme.seriesColors[0]
                                width: 2
                            }
                        }

                        RowLayout {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            spacing: 14

                            LegendItem {
                                markerColor: graphsTheme.seriesColors[0]
                                text: qsTr("TTFT")
                            }
                        }
                    }
                }
            }

            Panel {
                title: qsTr("Token trend")
                detail: qsTr("Prompt miss · Completion · Cache hit")
                Layout.fillWidth: true; Layout.preferredHeight: 250
                Layout.leftMargin: 20; Layout.rightMargin: 20

                Item {
                    anchors.fill: parent

                    GraphsView {
                        anchors.fill: parent
                        axisX: BarCategoryAxis {
                            categories: summary.turnCategories
                        }
                        axisY: ValueAxis {
                            min: 0
                            max: Math.max(1, summary.tokenMaximum)
                        }
                        marginLeft: 0; marginTop: 0; marginRight: 0; marginBottom: 32
                        theme: graphsTheme

                        BarSeries {
                            barsType: BarSeries.BarsType.Stacked

                            BarSet {
                                label: qsTr("Prompt miss")
                                values: summary.promptMissValues
                            }

                            BarSet {
                                label: qsTr("Completion")
                                values: summary.completionValues
                            }

                            BarSet {
                                label: qsTr("Cache hit")
                                values: summary.cacheValues
                            }
                        }
                    }

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        spacing: 14

                        LegendItem {
                            markerColor: graphsTheme.seriesColors[0]
                            text: qsTr("Prompt miss")
                        }

                        LegendItem {
                            markerColor: graphsTheme.seriesColors[1]
                            text: qsTr("Completion")
                        }

                        LegendItem {
                            markerColor: graphsTheme.seriesColors[2]
                            text: qsTr("Cache hit")
                        }
                    }
                }
            }

            Panel {
                title: qsTr("Execution trace")
                detail: qsTr("Turns and messages")
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 20

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        color: global.back
                        Layout.fillWidth: true; Layout.preferredHeight: 24

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            HeaderLabel {
                                text: qsTr("Trace")
                                Layout.preferredWidth: turnHistory.width * 0.4
                            }

                            HeaderLabel {
                                text: qsTr("Duration")
                                Layout.preferredWidth: turnHistory.width * 0.2
                            }

                            HeaderLabel {
                                text: qsTr("TTFT")
                                Layout.preferredWidth: turnHistory.width * 0.2
                            }

                            HeaderLabel {
                                text: qsTr("Tokens")
                                Layout.preferredWidth: turnHistory.width * 0.2
                            }
                        }
                    }

                    TreeView {
                        id: turnHistory
                        clip: true
                        model: evalModel
                        reuseItems: false
                        Layout.fillWidth: true; Layout.fillHeight: true
                        property string selectedNodeId: ""
                        property string selectedConversationId: ""
                        property string hoveredNodeId: ""
                        property string selectedTitle: ""
                        property string selectedDurationText: ""
                        property real selectedDuration: 0
                        property real selectedToolDuration: 0

                        function clearSelection(): void {
                            selectedNodeId = ""
                            selectedTitle = ""
                            selectedDurationText = ""
                            selectedDuration = 0
                            selectedToolDuration = 0
                        }

                        function selectNode(nodeId: string, title: string, durationText: string, duration: real, toolDuration: real, toggle: bool): void {
                            if (toggle && selectedNodeId === nodeId) {
                                clearSelection()
                                return
                            }
                            selectedNodeId = nodeId
                            selectedTitle = title
                            selectedDurationText = durationText
                            selectedDuration = duration
                            selectedToolDuration = toolDuration
                        }

                        columnWidthProvider: function (column: int): real {
                            return column === 0 ? width * 0.4 : width * 0.2
                        }

                        Component.onCompleted: selectedConversationId = rootItem.summary.conversationId

                        ScrollBar.vertical: ScrollBar {
                            minimumSize: Math.min(1, 20 / Math.max(1, height))
                            policy: ScrollBar.AsNeeded
                            palette {
                                mid: global.stroke
                                dark: global.strokePressed
                            }
                        }

                        delegate: DelegateChooser {
                            role: "messageRole"

                            DelegateChoice {
                                column: 0
                                roleValue: "turn"
                                delegate: Item {
                                    id: turnDelegate
                                    required property bool isTreeNode
                                    required property bool expanded
                                    required property bool hasChildren
                                    required property int depth
                                    required property int row
                                    required property string display
                                    required property string nodeId
                                    required property int status
                                    required property real durationValue
                                    required property real toolDurationValue
                                    implicitWidth: turnHistory.width * 0.4
                                    implicitHeight: 24

                                    Rectangle {
                                        anchors.fill: parent
                                        color: global.backHover
                                        opacity: turnHistory.hoveredNodeId === turnDelegate.nodeId ? 1 : 0

                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: 150
                                            }
                                        }
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: turnHistory.selectedNodeId === turnDelegate.nodeId ? global.backSelected : "transparent"
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        Item {
                                            Layout.preferredWidth: 12; Layout.fillHeight: true

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 3; height: 14
                                                radius: 1.5
                                                color: rootItem.statusColor(turnDelegate.status)
                                            }
                                        }

                                        Item {
                                            Layout.preferredWidth: turnDelegate.depth * 24; Layout.fillHeight: true
                                        }

                                        Item {
                                            Layout.preferredWidth: 24; Layout.fillHeight: true

                                            IconImage {
                                                anchors.centerIn: parent
                                                width: 16; height: 16
                                                source: turnDelegate.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                                color: global.fore
                                                visible: turnDelegate.isTreeNode && turnDelegate.hasChildren
                                            }

                                            TapHandler {
                                                acceptedButtons: Qt.LeftButton
                                                gesturePolicy: TapHandler.DragWithinBounds

                                                onTapped: {
                                                    turnHistory.selectNode(
                                                        turnDelegate.nodeId,
                                                        turnDelegate.display,
                                                        evalModel.data(turnHistory.index(turnDelegate.row, 1)),
                                                        turnDelegate.durationValue,
                                                        turnDelegate.toolDurationValue,
                                                        false
                                                    )
                                                    turnHistory.toggleExpanded(turnDelegate.row)
                                                }
                                            }
                                        }

                                        Label {
                                            text: turnDelegate.display
                                            leftPadding: 2
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true; Layout.fillHeight: true
                                        }
                                    }

                                    HoverHandler {
                                        onHoveredChanged: {
                                            if (hovered) turnHistory.hoveredNodeId = turnDelegate.nodeId
                                            else if (turnHistory.hoveredNodeId === turnDelegate.nodeId) turnHistory.hoveredNodeId = ""
                                        }
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        gesturePolicy: TapHandler.DragWithinBounds

                                        onTapped: turnHistory.selectNode(
                                            turnDelegate.nodeId,
                                            turnDelegate.display,
                                            evalModel.data(turnHistory.index(turnDelegate.row, 1)),
                                            turnDelegate.durationValue,
                                            turnDelegate.toolDurationValue,
                                            true
                                        )
                                    }

                                    Component.onCompleted: {
                                        if (turnHistory.selectedNodeId === nodeId) {
                                            turnHistory.selectNode(
                                                nodeId,
                                                display,
                                                evalModel.data(turnHistory.index(row, 1)),
                                                durationValue,
                                                toolDurationValue,
                                                false
                                            )
                                        }
                                    }
                                }
                            }

                            DelegateChoice {
                                column: 0
                                delegate: Item {
                                    id: messageDelegate
                                    required property int depth
                                    required property int row
                                    required property string display
                                    required property url decoration
                                    required property string nodeId
                                    required property real durationValue
                                    required property real toolDurationValue
                                    implicitWidth: turnHistory.width * 0.4
                                    implicitHeight: 24

                                    Rectangle {
                                        anchors.fill: parent
                                        color: global.backHover
                                        opacity: turnHistory.hoveredNodeId === messageDelegate.nodeId ? 1 : 0

                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: 150
                                            }
                                        }
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: turnHistory.selectedNodeId === messageDelegate.nodeId ? global.backSelected : "transparent"
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        Item {
                                            Layout.preferredWidth: 12; Layout.fillHeight: true
                                        }

                                        Item {
                                            Layout.preferredWidth: messageDelegate.depth * 24; Layout.fillHeight: true
                                        }

                                        Item {
                                            Layout.preferredWidth: 24; Layout.fillHeight: true

                                            IconImage {
                                                anchors.centerIn: parent
                                                width: 16; height: 16
                                                source: messageDelegate.decoration
                                                color: global.fore
                                            }
                                        }

                                        Label {
                                            text: messageDelegate.display
                                            leftPadding: 2
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true; Layout.fillHeight: true
                                        }
                                    }

                                    HoverHandler {
                                        onHoveredChanged: {
                                            if (hovered) turnHistory.hoveredNodeId = messageDelegate.nodeId
                                            else if (turnHistory.hoveredNodeId === messageDelegate.nodeId) turnHistory.hoveredNodeId = ""
                                        }
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        gesturePolicy: TapHandler.DragWithinBounds

                                        onTapped: turnHistory.selectNode(
                                            messageDelegate.nodeId,
                                            messageDelegate.display,
                                            evalModel.data(turnHistory.index(messageDelegate.row, 1)),
                                            messageDelegate.durationValue,
                                            messageDelegate.toolDurationValue,
                                            true
                                        )
                                    }

                                    Component.onCompleted: {
                                        if (turnHistory.selectedNodeId === nodeId) {
                                            turnHistory.selectNode(
                                                nodeId,
                                                display,
                                                evalModel.data(turnHistory.index(row, 1)),
                                                durationValue,
                                                toolDurationValue,
                                                false
                                            )
                                        }
                                    }
                                }
                            }

                            DelegateChoice {
                                delegate: Item {
                                    id: valueDelegate
                                    required property string display
                                    required property string nodeId
                                    implicitWidth: turnHistory.width * 0.2
                                    implicitHeight: 24

                                    Rectangle {
                                        anchors.fill: parent
                                        color: global.backHover
                                        opacity: turnHistory.hoveredNodeId === valueDelegate.nodeId ? 1 : 0

                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: 150
                                            }
                                        }
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: turnHistory.selectedNodeId === valueDelegate.nodeId ? global.backSelected : "transparent"
                                    }

                                    Label {
                                        anchors.fill: parent
                                        text: valueDelegate.display
                                        leftPadding: 6
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    HoverHandler {
                                        onHoveredChanged: {
                                            if (hovered) turnHistory.hoveredNodeId = valueDelegate.nodeId
                                            else if (turnHistory.hoveredNodeId === valueDelegate.nodeId) turnHistory.hoveredNodeId = ""
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: evalModel

        function onChangeSummary(): void {
            rootItem.ttftUpdate()
            const conversationId = rootItem.summary.conversationId
            if (turnHistory.selectedConversationId !== conversationId) {
                turnHistory.clearSelection()
                turnHistory.selectedConversationId = conversationId
            }
        }
    }

    component StatCard: Rectangle {
        id: statCard
        required property string title
        required property string value
        required property string detail
        implicitHeight: 92
        color: global.back
        border.color: global.stroke
        radius: 6

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 2

            Label {
                text: statCard.title
                color: global.foreDisabled
            }

            Label {
                text: statCard.value
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            Label {
                text: statCard.detail
                color: global.foreDisabled
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    component Panel: Rectangle {
        id: panel
        default property alias content: body.data
        required property string title
        required property string detail
        color: global.back
        border.color: global.stroke
        radius: 6

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: panel.title
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                Label {
                    text: panel.detail
                    color: global.foreDisabled
                }
            }

            Item {
                id: body
                Layout.fillWidth: true; Layout.fillHeight: true
            }
        }
    }

    component LegendItem: RowLayout {
        id: legendItem
        required property color markerColor
        required property string text
        spacing: 5

        Rectangle {
            radius: 2
            color: legendItem.markerColor
            Layout.preferredWidth: 8; Layout.preferredHeight: 8
        }

        Label {
            text: legendItem.text
            color: global.foreDisabled
        }
    }

    component HeaderLabel: Label {
        color: global.foreDisabled
        leftPadding: 6
        verticalAlignment: Text.AlignVCenter
        Layout.fillHeight: true
    }
}

import QtGraphs
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent
    property bool resize: true

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    RowLayout {
        anchors.fill: parent

        GraphsTheme {
            id: graphsTheme
            theme: GraphsTheme.Theme.QtGreen
            backgroundVisible: false
            plotAreaBackgroundVisible: false
            gridVisible: false
        }

        StackLayout {
            currentIndex: graphTypeComboBox.currentIndex
            clip: true
            Layout.fillWidth: true; Layout.fillHeight: true

            GraphsView {
                axisX: BarCategoryAxis {
                    categories: [""]
                    // alignment : barOrientationSwitch.checked ? Qt.AlignLeft : Qt.AlignBottom
                }
                axisY: ValueAxis {
                    id: barAxisY
                    property real minHint: 0
                    property real maxHint: 0
                }
                marginLeft: 10; marginTop: 10; marginRight: 10; marginBottom: 10
                orientation: barOrientationSwitch.checked ? Qt.Horizontal : Qt.Vertical
                theme: graphsTheme

                BarSeries {
                    id: barSeries
                    barWidth: count * 0.1
                    property var barSetMap: ({})

                    onCountChanged: {
                        barLegend.reload()
                    }

                    Component {
                        id: barComponent

                        BarSet {
                        }
                    }

                    function barInsert(row) {
                        const keyIndex = databaseStandardItemModel.index(row, 0);
                        const key = databaseStandardItemModel.data(keyIndex, Qt.DisplayRole)
                        databaseStandardItemModel.setData(keyIndex, true, Qt.WhatsThisRole)
                        const valueIndex = databaseStandardItemModel.index(row, 1);
                        const value = databaseStandardItemModel.data(valueIndex, Qt.DisplayRole)
                        const bar = barComponent.createObject(barSeries, {
                            label: key,
                            values: [value]
                        });
                        append(bar)
                        barSetMap[key] = bar
                    }

                    function barRemove(row) {
                        const keyIndex = databaseStandardItemModel.index(row, 0);
                        const key = databaseStandardItemModel.data(keyIndex, Qt.DisplayRole)
                        databaseStandardItemModel.setData(keyIndex, false, Qt.WhatsThisRole)
                        const bar = barSetMap[key]
                        remove(bar)
                        delete barSetMap[key]
                    }
                }

                Popup {
                    id: barLegend
                    width: 120; height: barLegend.model.count * 20 + 30
                    closePolicy: Popup.NoAutoClose
                    modal: false
                    background.opacity: 0.3
                    visible: barSeries.count
                    property var model: ListModel
                    {
                    }

                    ListView {
                        anchors.fill: parent
                        interactive: false
                        model: barLegend.model
                        delegate: Item {
                            width: 90; height: 20

                            RowLayout {
                                anchors.fill: parent
                                Layout.alignment: Qt.AlignVCenter

                                Rectangle {
                                    width: 14; height: 14
                                    color: model.color
                                }

                                Label {
                                    text: model.label
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    function reload() {
                        barLegend.model.clear()
                        for (let i = 0; i < barSeries.barSets.length; ++i) {
                            barLegend.model.append({
                                label: barSeries.barSets[i].label,
                                color: graphsTheme.seriesColors[i % graphsTheme.seriesColors.length]
                            })
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        preventStealing: true
                        property real lastX
                        property real lastY

                        onPressed: (mouse) => {
                            lastX = mouse.x
                            lastY = mouse.y
                        }

                        onPositionChanged: (mouse) => {
                            barLegend.x += mouse.x - lastX
                            barLegend.y += mouse.y - lastY
                        }
                    }
                }
            }

            GraphsView {
                id: lineGraph
                axisX: lineAxisSwitch.checked ? lineAxisTime : lineAxisIndex
                axisY: ValueAxis {
                    id: lineAxisY
                    property real minHint: 0
                    property real maxHint: 0
                }
                focus: true
                marginLeft: 10; marginTop: 10; marginRight: 10; marginBottom: 10
                theme: graphsTheme
                panStyle: GraphsView.PanStyle.None
                zoomAreaEnabled: true
                zoomStyle: GraphsView.ZoomStyle.Center
                property var lineSeriesMap: ({})

                ValueAxis {
                    id: lineAxisIndex
                    titleText: qsTr("index")
                    property real maxHint: 0
                }

                ValueAxis {
                    id: lineAxisTime
                    titleText: qsTr("time")
                    property real maxHint: 0
                    property real baseTime
                }

                onVisibleChanged: {
                    if (visible) forceActiveFocus()
                }

                HoverHandler {
                    id: hoverHandler
                    cursorShape: Qt.ArrowCursor
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onPressedChanged: parent.forceActiveFocus()
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Alt) {
                        hoverHandler.cursorShape = Qt.OpenHandCursor
                        panStyle = GraphsView.PanStyle.Drag
                        zoomAreaEnabled = false
                    }
                }

                Keys.onReleased: (event) => {
                    if (event.key === Qt.Key_Alt) {
                        hoverHandler.cursorShape = Qt.ArrowCursor
                        panStyle = GraphsView.PanStyle.None
                        zoomAreaEnabled = true
                    }
                }

                Component {
                    id: lineComponent

                    LineSeries {
                        id: lineSeries
                        width: 3
                        property string label
                    }
                }

                function lineInsert(row) {
                    const keyIndex = datatableHeaderItemModel.index(row, 0);
                    const key = datatableHeaderItemModel.data(keyIndex, Qt.DisplayRole)
                    datatableHeaderItemModel.setData(keyIndex, true, Qt.WhatsThisRole)
                    const line = lineComponent.createObject(lineGraph, {
                        label: key
                    });
                    addSeries(line)
                    lineSeriesMap[key] = line
                    lineLegend.reload()

                    if (!lineAxisSwitch.checked) {
                        // index based
                        const points = [];
                        for (let i = 0; i < datatableStandardItemModel.rowCount(); ++i) {
                            const valueIndex = datatableStandardItemModel.index(i, row);
                            const value = datatableStandardItemModel.data(valueIndex, Qt.DisplayRole)
                            if (value === undefined) {
                                break
                            } else {
                                points.push(Qt.point(i, value))
                                if (i > lineAxisIndex.maxHint) {
                                    lineAxisIndex.maxHint = i
                                }
                                if (value < lineAxisY.minHint * 0.8) {
                                    lineAxisY.minHint = value / 0.8
                                } else if (value > lineAxisY.maxHint * 0.8) {
                                    lineAxisY.maxHint = value / 0.8
                                }
                            }
                        }
                        line.append(points)
                        if (rootItem.resize) {
                            graphResize(1)
                        }
                    }
                }

                function lineRemove(row) {
                    const keyIndex = datatableHeaderItemModel.index(row, 0);
                    const key = datatableHeaderItemModel.data(keyIndex, Qt.DisplayRole)
                    datatableHeaderItemModel.setData(keyIndex, false, Qt.WhatsThisRole)
                    const line = lineSeriesMap[key]
                    removeSeries(line)
                    delete lineSeriesMap[key]
                    lineLegend.reload()
                }

                Popup {
                    id: lineLegend
                    width: 120; height: lineLegend.model.count * 20 + 30
                    closePolicy: Popup.NoAutoClose
                    modal: false
                    background.opacity: 0.3
                    property var model: ListModel
                    {
                    }

                    ListView {
                        anchors.fill: parent
                        interactive: false
                        model: lineLegend.model
                        delegate: Item {
                            width: 90; height: 20

                            RowLayout {
                                anchors.fill: parent
                                Layout.alignment: Qt.AlignVCenter

                                Rectangle {
                                    width: 14; height: 14
                                    color: model.color
                                }

                                Label {
                                    text: model.label
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    function reload() {
                        lineLegend.visible = lineGraph.seriesList.length
                        lineLegend.model.clear()
                        for (let i = 0; i < lineGraph.seriesList.length; ++i) {
                            lineLegend.model.append({
                                label: lineGraph.seriesList[i].label,
                                color: graphsTheme.seriesColors[i % graphsTheme.seriesColors.length]
                            })
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        preventStealing: true
                        property real lastX
                        property real lastY

                        onPressed: (mouse) => {
                            lastX = mouse.x
                            lastY = mouse.y
                        }

                        onPositionChanged: (mouse) => {
                            lineLegend.x += mouse.x - lastX
                            lineLegend.y += mouse.y - lastY
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 200; Layout.fillHeight: true
            Layout.maximumWidth: 200
            Layout.alignment: Qt.AlignTop

            ComboBox {
                id: graphTypeComboBox
                model: ListModel {
                    ListElement {
                        text: qsTr("Database(Bar)"); value: "bar"
                    }
                    ListElement {
                        text: qsTr("Datatable(Line)"); value: "line"
                    }
                }
                textRole: "text"
                valueRole: "value"
                Layout.fillWidth: true
            }

            StackLayout {
                currentIndex: graphTypeComboBox.currentIndex

                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        Label {
                            text: qsTr("vertical")
                        }

                        Switch {
                            id: barOrientationSwitch
                        }

                        Label {
                            text: qsTr("horizontal")
                        }
                    }

                    TableView {
                        id: databaseTableView
                        Layout.fillWidth: true; Layout.fillHeight: true
                        alternatingRows: false
                        clip: true
                        editTriggers: TableView.NoEditTriggers
                        rowSpacing: 1
                        model: databaseStandardItemModel
                        contentWidth: width

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

                        delegate: DelegateChooser {
                            DelegateChoice {
                                column: 0
                                delegate: CheckDelegate {
                                    implicitWidth: databaseTableView.width
                                    checked: model.whatsThis
                                    text: model.display
                                    background: Rectangle {
                                        color: global.back
                                    }

                                    onClicked: {
                                        if (checked) {
                                            barSeries.barInsert(row)
                                        } else {
                                            barSeries.barRemove(row)
                                        }
                                    }
                                }
                            }
                            DelegateChoice {
                                delegate: Item {
                                    implicitWidth: 1
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        Label {
                            text: qsTr("index")
                        }

                        Switch {
                            id: lineAxisSwitch

                            onClicked: graphClear(1)
                        }

                        Label {
                            text: qsTr("time")
                        }
                    }

                    TableView {
                        id: datatableTableView
                        Layout.fillWidth: true; Layout.fillHeight: true
                        alternatingRows: false
                        clip: true
                        editTriggers: TableView.NoEditTriggers
                        rowSpacing: 1
                        model: datatableHeaderItemModel
                        contentWidth: width

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

                        delegate: CheckDelegate {
                            implicitWidth: datatableTableView.width
                            checked: model.whatsThis
                            text: model.display
                            background: Rectangle {
                                color: global.back
                            }

                            onClicked: {
                                if (checked) {
                                    lineGraph.lineInsert(row)
                                } else {
                                    lineGraph.lineRemove(row)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton

        onSingleTapped: {
            rootMenu.rootItem = rootItem
            rootMenu.popup()
        }
    }

    TapHandler {
        acceptedButtons: Qt.MiddleButton

        onSingleTapped: graphResize()
    }

    function graphResize(index = -1) {
        if (index === -1) index = graphTypeComboBox.currentIndex
        switch (index) {
            case 0: {
                if (barSeries.count === 0) {
                    barAxisY.min = 0
                    barAxisY.max = 10
                    barAxisY.minHint = 0
                    barAxisY.maxHint = 0
                } else {
                    barAxisY.min = barAxisY.minHint
                    barAxisY.max = barAxisY.maxHint
                }
                barAxisY.zoom = 1
                barAxisY.pan = 0
            }
                break
            case 1: {
                if (lineGraph.seriesList.length === 0) {
                    if (!lineAxisSwitch.checked) {
                        lineAxisIndex.max = 10
                        lineAxisIndex.maxHint = 0
                    } else {
                        lineAxisTime.max = 10
                        lineAxisTime.maxHint = 0
                    }
                    lineAxisY.min = 0
                    lineAxisY.max = 10
                    lineAxisY.minHint = 0
                    lineAxisY.maxHint = 0
                } else {
                    if (!lineAxisSwitch.checked) {
                        lineAxisIndex.max = lineAxisIndex.maxHint
                    } else {
                        lineAxisTime.max = lineAxisTime.maxHint
                    }
                    lineAxisY.min = lineAxisY.minHint
                    lineAxisY.max = lineAxisY.maxHint
                }
                lineAxisIndex.zoom = 1
                lineAxisIndex.pan = 0
                lineAxisTime.zoom = 1
                lineAxisTime.pan = 0
                lineAxisY.zoom = 1
                lineAxisY.pan = 0
            }
                break
        }
    }

    function graphClear(index = -1) {
        if (index === -1) index = graphTypeComboBox.currentIndex
        switch (index) {
            case 0: {
                for (let row = 0; row < databaseStandardItemModel.rowCount(); ++row) {
                    const keyIndex = databaseStandardItemModel.index(row, 0);
                    const watched = databaseStandardItemModel.data(keyIndex, Qt.WhatsThisRole)
                    if (watched) {
                        barSeries.barRemove(row)
                    }
                }
            }
                break
            case 1: {
                for (let row = 0; row < datatableHeaderItemModel.rowCount(); ++row) {
                    const keyIndex = datatableHeaderItemModel.index(row, 0);
                    const watched = datatableHeaderItemModel.data(keyIndex, Qt.WhatsThisRole)
                    if (watched) {
                        lineGraph.lineRemove(row)
                    }
                }
            }
                break
        }
        graphResize(index)
    }

    Connections {
        target: databaseStandardItemModel

        function onDataChanged(topLeft, bottomRight, roles) {
            const col = topLeft.column
            if (col !== 1) return
            const row = topLeft.row
            const keyIndex = databaseStandardItemModel.index(row, 0);
            const watched = databaseStandardItemModel.data(keyIndex, Qt.WhatsThisRole)
            if (watched) {
                const key = databaseStandardItemModel.data(keyIndex, Qt.DisplayRole)
                const valueIndex = databaseStandardItemModel.index(row, 1);
                const value = databaseStandardItemModel.data(valueIndex, Qt.DisplayRole)
                barSeries.barSetMap[key].values = [value]
                // calc range
                if (value < barAxisY.minHint * 0.8) {
                    barAxisY.minHint = value / 0.8
                } else if (value > barAxisY.maxHint * 0.8) {
                    barAxisY.maxHint = value / 0.8
                }
                if (rootItem.resize) {
                    graphResize(0)
                }
            }
        }
    }

    Connections {
        target: datatableStandardItemModel

        function onDataChanged(topLeft, bottomRight, roles) {
            const col = topLeft.column
            const row = topLeft.row
            const keyIndex = datatableHeaderItemModel.index(col, 0);
            const watched = datatableHeaderItemModel.data(keyIndex, Qt.WhatsThisRole)
            if (watched) {
                const key = datatableHeaderItemModel.data(keyIndex, Qt.DisplayRole)
                const valueIndex = datatableStandardItemModel.index(row, col)
                const value = datatableStandardItemModel.data(valueIndex, Qt.DisplayRole)
                if (!lineAxisSwitch.checked) {
                    // index based
                    lineGraph.lineSeriesMap[key].append(row, value)
                    if (row > lineAxisIndex.maxHint) {
                        lineAxisIndex.maxHint = row
                    }
                } else {
                    // time based
                    if (lineGraph.lineSeriesMap[key].count === 0) {
                        lineAxisTime.baseTime = new Date()
                    }
                    const timeElapsed = (new Date() - lineAxisTime.baseTime) / 1000
                    lineGraph.lineSeriesMap[key].append(timeElapsed, value)
                    if (timeElapsed > lineAxisTime.maxHint) {
                        lineAxisTime.maxHint = timeElapsed
                    }
                }
                // calc range
                if (value < lineAxisY.minHint * 0.8) {
                    lineAxisY.minHint = value / 0.8
                } else if (value > lineAxisY.maxHint * 0.8) {
                    lineAxisY.maxHint = value / 0.8
                }
                if (rootItem.resize) {
                    graphResize(1)
                }
            }
        }
    }
}

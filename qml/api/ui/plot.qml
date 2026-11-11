import QtGraphs
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem

    required property var keys
    property bool autoFit: true
    property real minX: Infinity
    property real maxX: -Infinity
    property real minY: Infinity
    property real maxY: -Infinity

    Window.onActiveChanged: {
        if (rootItem.Window.active) graph.forceActiveFocus()
        else graph.altPressed = false
    }

    Component.onCompleted: {
        for (const key of rootItem.keys) {
            graph.addSeries(lineComponent.createObject(graph, {name: key}))
        }
        legendRepeater.model = graph.seriesList
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        GraphsView {
            id: graph
            Layout.fillWidth: true; Layout.fillHeight: true
            focus: true
            property bool altPressed: false
            marginLeft: 10; marginTop: 10; marginRight: 10; marginBottom: 10
            zoomStyle: GraphsView.ZoomStyle.Center
            zoomAreaEnabled: !altPressed
            panStyle: altPressed ? GraphsView.PanStyle.Drag : GraphsView.PanStyle.None

            theme: GraphsTheme {
                theme: GraphsTheme.Theme.QtGreen
                backgroundVisible: false
                plotAreaBackgroundVisible: false
                gridVisible: false
            }

            axisX: ValueAxis {
                id: axisX
                min: 0; max: 1
                onZoomChanged: if (zoom !== 1) rootItem.autoFit = false
                onPanChanged: if (pan !== 0) rootItem.autoFit = false
            }
            axisY: ValueAxis {
                id: axisY
                min: 0; max: 1
                onZoomChanged: if (zoom !== 1) rootItem.autoFit = false
                onPanChanged: if (pan !== 0) rootItem.autoFit = false
            }

            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Alt) {
                    graph.altPressed = true
                    event.accepted = true
                }
            }
            Keys.onReleased: (event) => {
                if (event.key === Qt.Key_Alt) {
                    graph.altPressed = false
                    event.accepted = true
                }
            }

            HoverHandler {
                cursorShape: graph.altPressed ? Qt.OpenHandCursor : Qt.ArrowCursor
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onPressedChanged: if (pressed) graph.forceActiveFocus()
            }
            TapHandler {
                acceptedButtons: Qt.MiddleButton
                onTapped: rootItem.fit()
            }
        }

        Flow {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 12

            move: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }

            Repeater {
                id: legendRepeater
                delegate: Row {
                    id: legendSeries
                    required property var modelData

                    Repeater {
                        model: legendSeries.modelData.legendData
                        delegate: Row {
                            id: legendEntry
                            required property var modelData
                            spacing: 6

                            Rectangle {
                                width: 12; height: 12
                                anchors.verticalCenter: parent.verticalCenter
                                color: legendEntry.modelData.color
                            }
                            Label {
                                text: legendEntry.modelData.label
                                textFormat: Text.PlainText
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: lineComponent
        LineSeries { width: 2 }
    }

    function append(x, values) {
        // append supplied series and track data bounds
        for (const key in values) {
            const y = values[key]
            graph.seriesList[rootItem.keys.indexOf(key)].append(x, y)
            rootItem.minX = Math.min(rootItem.minX, x)
            rootItem.maxX = Math.max(rootItem.maxX, x)
            rootItem.minY = Math.min(rootItem.minY, y)
            rootItem.maxY = Math.max(rootItem.maxY, y)
        }
        if (rootItem.autoFit) rootItem.fit()
    }

    function fit() {
        // reset navigation and resume automatic fitting
        axisX.zoom = 1; axisX.pan = 0
        axisY.zoom = 1; axisY.pan = 0
        rootItem.autoFit = true
        if (rootItem.minX === Infinity) return

        const paddingX = (rootItem.maxX - rootItem.minX || 1) * 0.05
        const paddingY = (rootItem.maxY - rootItem.minY || 1) * 0.05
        axisX.min = rootItem.minX - paddingX
        axisX.max = rootItem.maxX + paddingX
        axisY.min = rootItem.minY - paddingY
        axisY.max = rootItem.maxY + paddingY
    }
}

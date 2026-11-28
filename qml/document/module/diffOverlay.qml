pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes

import UniComm

Item {
    id: root
    required property QtObject diffOverlay
    readonly property var regions: diffOverlay.regions

    Repeater {
        model: root.regions.length

        delegate: Item {
            id: region
            required property int index
            readonly property var geometry: root.regions[index]
            readonly property real curveStart: geometry.curveStart
            readonly property real curveEnd: geometry.curveEnd
            readonly property real middle: (curveStart + curveEnd) / 2
            x: geometry.area.x; y: geometry.area.y
            width: geometry.area.width; height: geometry.area.height
            clip: true

            Repeater {
                model: region.geometry.bands.length

                delegate: Shape {
                    id: band
                    required property int index
                    readonly property var span: region.geometry.bands[index]
                    readonly property var colors: DiffStyle.diffColor(span.kind)
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer

                    ShapePath {
                        fillColor: Qt.alpha(band.colors.back, 0.5)
                        strokeColor: band.colors.fore
                        strokeWidth: 2
                        // Close outside the clip so only the top and bottom edges are visible.
                        startX: -2; startY: band.span.topLeft - 1
                        PathLine { x: region.curveStart; y: band.span.topLeft - 1 }
                        PathCubic {
                            x: region.curveEnd; y: band.span.topRight - 1
                            control1X: region.middle; control1Y: band.span.topLeft - 1
                            control2X: region.middle; control2Y: band.span.topRight - 1
                        }
                        PathLine { x: region.width + 2; y: band.span.topRight - 1 }
                        PathLine { x: region.width + 2; y: band.span.bottomRight - 1 }
                        PathLine { x: region.curveEnd; y: band.span.bottomRight - 1 }
                        PathCubic {
                            x: region.curveStart; y: band.span.bottomLeft - 1
                            control1X: region.middle; control1Y: band.span.bottomRight - 1
                            control2X: region.middle; control2Y: band.span.bottomLeft - 1
                        }
                        PathLine { x: -2; y: band.span.bottomLeft - 1 }
                        PathLine { x: -2; y: band.span.topLeft - 1 }
                    }
                }
            }
        }
    }
}

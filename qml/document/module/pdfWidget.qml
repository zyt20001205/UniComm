import QtQuick
import QtQuick.Pdf

Item {
    id: root
    required property url source
    anchors.fill: parent

    PdfMultiPageView {
        anchors.fill: parent
        document: PdfDocument {
            source: root.source
        }
    }
}

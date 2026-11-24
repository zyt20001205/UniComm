pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: root
    required property QtObject toastModule
    width: 360
    height: activeToasts.count > 0 ? Math.max(1, toastColumn.implicitHeight) : 0

    readonly property int errorLevel: 0
    readonly property int warningLevel: 1
    readonly property int infoLevel: 2
    readonly property int successLevel: 3
    readonly property int maximumVisible: 5
    readonly property int motionDuration: 200
    property int nextId

    ListModel {
        id: activeToasts
    }

    Column {
        id: toastColumn
        width: parent.width
        spacing: 8

        Repeater {
            model: activeToasts

            delegate: Item {
                id: toastDelegate
                required property int toastId
                required property int toastLevel
                required property string toastTitle
                required property string toastText
                required property int toastDuration
                required property var toastActions
                required property int toastActionGroupId
                width: toastColumn.width
                height: toastPopup.implicitHeight
                property real remaining: 1
                property real popupOpacity: 0
                property bool closing: false
                readonly property url iconSource: toastLevel === root.errorLevel ? "qrc:/icon/error.svg"
                                                  : toastLevel === root.warningLevel ? "qrc:/icon/warning.svg"
                                                  : toastLevel === root.infoLevel ? "qrc:/icon/info.svg"
                                                  : "qrc:/icon/checkmarkCircle.svg"
                readonly property color levelColor: toastLevel === root.errorLevel ? global.theme.general.dangerBack3
                                                    : toastLevel === root.warningLevel ? global.theme.general.warningBack3
                                                    : toastLevel === root.infoLevel ? global.theme.general.brandBack
                                                    : global.theme.general.successBack3

                Behavior on y {
                    NumberAnimation {
                        duration: root.motionDuration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [0.33, 0, 0.67, 1, 1, 1]
                    }
                }

                ToolTip {
                    id: toastPopup
                    parent: toastDelegate
                    y: 0
                    width: toastDelegate.width
                    padding: 0
                    closePolicy: Popup.NoAutoClose
                    popupType: Popup.Item
                    delay: 0
                    timeout: -1
                    visible: true
                    opacity: toastDelegate.popupOpacity
                    enter: null
                    exit: null

                    contentItem: ColumnLayout {
                        id: toastContent
                        spacing: 0

                        HoverHandler {
                            onHoveredChanged: {
                                if (toastDelegate.closing) return
                                if (hovered) progressAnimation.pause()
                                else progressAnimation.resume()
                            }
                        }

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true
                            Layout.leftMargin: 10
                            Layout.rightMargin: 10
                            Layout.topMargin: 10
                            Layout.bottomMargin: 8

                            IconImage {
                                source: toastDelegate.iconSource
                                color: toastDelegate.levelColor
                                fillMode: Image.PreserveAspectFit
                                sourceSize.width: 20; sourceSize.height: 20
                                Layout.preferredWidth: 20; Layout.preferredHeight: 20
                                Layout.alignment: Qt.AlignTop
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                Label {
                                    text: toastDelegate.toastTitle
                                    color: global.theme.general.fore
                                    font.bold: true
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    visible: toastDelegate.toastTitle.length > 0
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: toastDelegate.toastText
                                    color: global.theme.general.fore
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    visible: toastDelegate.toastText.length > 0
                                    Layout.fillWidth: true
                                }

                                Flow {
                                    spacing: 12
                                    visible: toastDelegate.toastActionGroupId >= 0
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4

                                    Repeater {
                                        model: toastDelegate.toastActions

                                        delegate: Button {
                                            id: actionButton
                                            required property string actionText
                                            required property int actionIndex

                                            flat: true
                                            hoverEnabled: true
                                            focusPolicy: Qt.NoFocus
                                            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                            text: actionText

                                            background: Item {}

                                            contentItem: Label {
                                                text: actionButton.text
                                                color: actionButton.down ? global.theme.general.brandBackSelected
                                                                         : actionButton.hovered ? global.theme.general.brandBack
                                                                                                : global.theme.general.brandLink
                                                font.underline: actionButton.hovered
                                                horizontalAlignment: Text.AlignLeft
                                                verticalAlignment: Text.AlignVCenter
                                            }

                                            HoverHandler {
                                                cursorShape: Qt.PointingHandCursor
                                            }

                                            onClicked: toastDelegate.triggerAction(actionIndex)
                                        }
                                    }
                                }
                            }

                            Button {
                                flat: true
                                focusPolicy: Qt.NoFocus
                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                icon.source: "qrc:/icon/dismiss.svg"
                                icon.width: 12; icon.height: 12
                                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                                Layout.alignment: Qt.AlignTop

                                onClicked: toastDelegate.beginDismiss()
                            }
                        }

                        ProgressBar {
                            from: 0
                            to: 1
                            value: toastDelegate.remaining
                            palette.accent: toastDelegate.levelColor
                            Layout.fillWidth: true
                            Layout.leftMargin: 2; Layout.rightMargin: 2
                        }
                    }
                }

                NumberAnimation {
                    id: enterAnimation
                    target: toastDelegate
                    property: "popupOpacity"
                    from: 0
                    to: 1
                    duration: root.motionDuration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.33, 0, 0.67, 1, 1, 1]
                }

                NumberAnimation {
                    id: progressAnimation
                    target: toastDelegate
                    property: "remaining"
                    from: 1
                    to: 0
                    duration: toastDelegate.toastDuration

                    onFinished: toastDelegate.beginDismiss()
                }

                NumberAnimation {
                    id: exitAnimation
                    target: toastDelegate
                    property: "popupOpacity"
                    to: 0
                    duration: root.motionDuration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: [0.33, 0, 0.67, 1, 1, 1]

                    onFinished: root.dismiss(toastDelegate.toastId)
                }

                function beginDismiss(): void {
                    if (closing) return
                    closing = true
                    progressAnimation.stop()
                    exitAnimation.start()
                }

                function triggerAction(actionIndex: int): void {
                    if (closing || toastActionGroupId < 0) return
                    const actionGroupId = toastActionGroupId
                    beginDismiss()
                    root.toastModule.actionTrigger(actionGroupId, actionIndex)
                }

                Component.onCompleted: {
                    enterAnimation.start()
                    if (toastDuration > 0) progressAnimation.start()
                }
            }
        }
    }

    function show(level: int, title: string, text: string, actions: var, actionGroupId: int, duration: int): void {
        const toast = {
            "toastId": ++nextId,
            "toastLevel": level,
            "toastTitle": title,
            "toastText": text,
            "toastDuration": duration,
            "toastActions": actions,
            "toastActionGroupId": actionGroupId
        }
        if (activeToasts.count >= maximumVisible) removeAt(0)
        activeToasts.append(toast)
    }

    function removeAt(index: int): void {
        root.toastModule.actionRemove(activeToasts.get(index).toastActionGroupId)
        activeToasts.remove(index)
    }

    function dismiss(toastId: int): void {
        for (let index = 0; index < activeToasts.count; ++index) {
            if (activeToasts.get(index).toastId === toastId) {
                removeAt(index)
                break
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Window

Item {
    id: control
    property string iconName
    property color iconColor: "#777777"
    property string tooltip
    property real uiScale: 1
    property bool enabled: true
    signal clicked()

    width: Math.round(16 * uiScale)
    height: Math.round(16 * uiScale)
    opacity: enabled ? 1 : 0.3
    ToolTip.visible: hitArea.containsMouse && tooltip.length > 0
    ToolTip.text: tooltip

    Canvas {
        id: canvas
        readonly property real dpr: Screen.devicePixelRatio
        width: control.width * dpr
        height: control.height * dpr
        transformOrigin: Item.TopLeft
        scale: 1 / dpr
        onDprChanged: requestPaint()
        onPaint: {
            const c = getContext("2d")
            c.setTransform(dpr * control.uiScale, 0, 0, dpr * control.uiScale, 0, 0)
            c.clearRect(0, 0, 16, 16)
            c.strokeStyle = control.iconColor
            c.lineWidth = 1.4
            c.lineCap = "round"
            c.lineJoin = "round"
            c.beginPath()
            if (control.iconName === "folder") {
                c.moveTo(2.5, 13); c.lineTo(2.5, 3.5); c.lineTo(6.5, 3.5)
                c.lineTo(8.5, 5.5); c.lineTo(13.5, 5.5); c.lineTo(13.5, 13); c.closePath()
            } else if (control.iconName === "contents") {
                for (let y of [4, 8, 12]) { c.moveTo(2.5, y); c.lineTo(4, y); c.moveTo(6, y); c.lineTo(13.5, y) }
            } else if (control.iconName === "search") {
                c.arc(6.5, 6.5, 4, 0, Math.PI * 2)
                c.moveTo(9.5, 9.5); c.lineTo(13.5, 13.5)
            } else if (control.iconName === "minus") {
                c.moveTo(3, 8); c.lineTo(13, 8)
            } else if (control.iconName === "plus") {
                c.moveTo(3, 8); c.lineTo(13, 8); c.moveTo(8, 3); c.lineTo(8, 13)
            } else if (control.iconName === "fullscreen") {
                c.moveTo(2.5, 6); c.lineTo(2.5, 2.5); c.lineTo(6, 2.5)
                c.moveTo(10, 2.5); c.lineTo(13.5, 2.5); c.lineTo(13.5, 6)
                c.moveTo(13.5, 10); c.lineTo(13.5, 13.5); c.lineTo(10, 13.5)
                c.moveTo(6, 13.5); c.lineTo(2.5, 13.5); c.lineTo(2.5, 10)
            }
            c.stroke()
        }
        Connections {
            target: control
            function onIconColorChanged() { canvas.requestPaint() }
            function onIconNameChanged() { canvas.requestPaint() }
            function onUiScaleChanged() { canvas.requestPaint() }
        }
    }
    MouseArea {
        id: hitArea
        anchors.centerIn: parent
        width: Math.round(28 * control.uiScale)
        height: Math.round(28 * control.uiScale)
        hoverEnabled: true
        enabled: control.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: control.clicked()
    }
}

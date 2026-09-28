import QtQuick
import Quickshell
import qs.Commons
import qs.Ui

BarWidget {
  id: root
  moduleName: "dimasmufid.omabook"
  implicitWidth: Style.space(28)
  implicitHeight: barSize

  Text {
    anchors.centerIn: parent
    text: "󰂺"
    color: root.bar.barForeground
    font.family: root.bar.fontFamily
    font.pixelSize: Style.font.body
  }

  MouseArea {
    anchors.fill: parent
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    onClicked: Quickshell.execDetached(["gtk-launch", "omabook"])
    onEntered: if (root.bar) root.bar.showTooltip(root, "Open Omabook")
    onExited: if (root.bar) root.bar.hideTooltip(root)
  }
}

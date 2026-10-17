import QtQuick
import QtQuick.Controls
Button {
    id: control
    property color accent: "#378ADD"
    property color ink: "#5F5E5A"
    property color paper: "#FFFFFF"
    property color edge: "#D3D1C7"
    property bool primary: false
    implicitHeight: 34
    implicitWidth: Math.max(50, caption.implicitWidth + 28)
    opacity: enabled ? 1 : 0.45
    contentItem: Text { id: caption; text: control.text; color: control.primary ? "white" : control.ink; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
    background: Rectangle { radius: height / 2; color: control.primary ? control.accent : control.paper; border.color: control.activeFocus ? control.accent : control.edge }
    Accessible.name: text
}

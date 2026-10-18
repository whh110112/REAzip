import QtQuick
import QtQuick.Controls
Button {
    id: control
    property color accent: "#486451"
    property color ink: "#59645C"
    property color paper: "#FFFFFF"
    property color edge: "#D7D8CF"
    property bool primary: false
    property bool soft: false
    implicitHeight: 34
    implicitWidth: Math.max(50, caption.implicitWidth + 28)
    opacity: enabled ? 1 : 0.45
    contentItem: Text { id: caption; text: control.text; textFormat:Text.PlainText; color: control.primary ? (control.soft ? control.accent : "white") : control.ink; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
    background: Rectangle { radius: 7; color: control.primary ? (control.soft ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.15) : control.accent) : control.paper; border.color: control.activeFocus ? control.accent : (control.primary && control.soft ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.35) : control.edge) }
    Accessible.name: text
}

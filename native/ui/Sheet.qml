import QtQuick
import QtQuick.Controls
Dialog {
    id: sheet
    property color paper: "#FFFFFF"
    property color ink: "#2C2C2A"
    property color edge: "#D3D1C7"
    property var api: typeof backend !== "undefined" ? backend : null
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(580, parent ? parent.width - 40 : 580)
    modal: true; focus: true; padding: 22
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: sheet.paper; border.color: sheet.edge; radius: 14 }
    header: Item {
        implicitHeight: 52
        Label { anchors.left: parent.left; anchors.leftMargin: 22; anchors.verticalCenter: parent.verticalCenter; text: sheet.title; color: sheet.ink; font.pixelSize: 16; font.bold: true }
        ToolButton { anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "×"; onClicked: sheet.reject(); Accessible.name: "Close" }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: sheet.edge }
    }
    Overlay.modal: Rectangle { color: "#6130302C" }
    footer: DialogButtonBox {
        standardButtons: sheet.standardButtons
        onAccepted: sheet.accept()
        onRejected: sheet.reject()
        delegate: Button {
            text: { if(sheet.api){let epoch=sheet.api.revision;return DialogButtonBox.buttonRole===DialogButtonBox.AcceptRole?sheet.api.translate("confirm","OK"):sheet.api.translate("cancel","Cancel")}return DialogButtonBox.buttonRole===DialogButtonBox.AcceptRole?"OK":"Cancel" }
        }
    }
}

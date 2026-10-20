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
        implicitHeight: Math.max(52, sheetTitle.implicitHeight + 24)
        Label { id:sheetTitle;anchors.left: parent.left; anchors.leftMargin: 22;anchors.right:sheetClose.left;anchors.rightMargin:8; anchors.verticalCenter: parent.verticalCenter; text: sheet.title;wrapMode:Text.Wrap; color: sheet.ink; font.pixelSize: 16; font.bold: true }
        ToolButton { id:sheetClose;anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "×"; onClicked: sheet.reject(); Accessible.name: "Close" }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: sheet.edge }
    }
    Overlay.modal: Rectangle { color: "#6130302C" }
    footer: DialogButtonBox {
        standardButtons: sheet.standardButtons
        alignment: Qt.AlignRight
        padding: 14; spacing: 10
        onAccepted: sheet.accept()
        onRejected: sheet.reject()
        delegate: Button {
            implicitWidth: 96; implicitHeight: 34
            text: { if(sheet.api){let epoch=sheet.api.revision;return DialogButtonBox.buttonRole===DialogButtonBox.AcceptRole?sheet.api.translate("confirm","OK"):sheet.api.translate("cancel","Cancel")}return DialogButtonBox.buttonRole===DialogButtonBox.AcceptRole?"OK":"Cancel" }
        }
    }
}

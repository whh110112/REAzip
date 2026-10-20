import QtQuick

Rectangle {
    id: backdrop
    property var artwork: ({})
    property color patternColor: "#59645C"
    readonly property string pattern: artwork.pattern || "none"
    readonly property real patternOpacity: artwork.opacity === undefined ? 0.1 : artwork.opacity
    readonly property real spacing: artwork.spacing || 16
    color: "#FAF9F6"
    clip: true
    Image {
        anchors.fill: parent
        source: backdrop.artwork.texture || ""
        fillMode: Image.Tile
        opacity: backdrop.patternOpacity
        cache: true
        visible: source.toString().length > 0
        smooth: true
    }
    Canvas {
        id: patternCanvas
        anchors.fill: parent
        visible: backdrop.pattern !== "none"
        opacity: backdrop.patternOpacity
        onPaint: {
            const c=getContext("2d"), step=backdrop.spacing
            c.reset()
            c.strokeStyle=backdrop.patternColor; c.fillStyle=backdrop.patternColor; c.lineWidth=1
            if (backdrop.pattern === "dots") {
                for(let x=step/2;x<width;x+=step)for(let y=step/2;y<height;y+=step){c.beginPath();c.arc(x,y,1,0,Math.PI*2);c.fill()}
            } else if (backdrop.pattern === "lines") {
                c.beginPath();for(let x=-height;x<width;x+=step){c.moveTo(x,0);c.lineTo(x+height,height)}c.stroke()
            } else if (backdrop.pattern === "grid") {
                c.beginPath();for(let x=step/2;x<width;x+=step){c.moveTo(x,0);c.lineTo(x,height)}
                for(let y=step/2;y<height;y+=step){c.moveTo(0,y);c.lineTo(width,y)}c.stroke()
            }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }
    onPatternChanged: if(patternCanvas)patternCanvas.requestPaint()
    onPatternColorChanged: if(patternCanvas)patternCanvas.requestPaint()
    onSpacingChanged: if(patternCanvas)patternCanvas.requestPaint()
}

import QtQuick
import "LogoMath.js" as LogoMath

Item {
    id: logo
    implicitWidth: 64; implicitHeight: 64
    property real archiveRatio: -1
    property color accentColor: "#486451"
    property color strongColor: "#334A3A"
    property color textColor: "#292F2A"
    property color surfaceColor: "#FAF9F6"
    property color baseColor: "#FAF9F6"
    property color patternColor: "#59645C"
    property var artwork: ({})
    readonly property bool measured: LogoMath.measured(archiveRatio)
    readonly property real retainedFraction: LogoMath.retained(archiveRatio)
    readonly property real openingFraction: 1-retainedFraction
    readonly property bool small: Math.min(width,height)<=24
    readonly property color resolvedSurface: LogoMath.composite(surfaceColor,baseColor)
    readonly property color foregroundColor: LogoMath.foreground(accentColor,strongColor,textColor,resolvedSurface,small)
    Accessible.role: Accessible.Graphic
    Accessible.name: "ZENzip"
    ThemeBackdrop { anchors.fill:parent; color:logo.resolvedSurface; artwork:logo.artwork; patternColor:logo.patternColor }
    Canvas {
        id: ring
        anchors.fill: parent
        onPaint: {
            const c=getContext("2d"), side=Math.min(width,height), fraction=logo.retainedFraction
            c.reset()
            if(side<=0 || fraction<=0)return
            c.translate((width-side)/2,(height-side)/2);c.scale(side/256,side/256)
            const gap=(1-fraction)*Math.PI*2, start=-Math.PI/4+gap/2, end=start+fraction*Math.PI*2
            // Measurement mode uses radial terminals: sector angle and annulus
            // area both give exactly 40% open when retained size is 60%.
            // Without archive data, preserve A's original round-ended 48° gap.
            c.lineCap=logo.measured?"butt":"round"
            const stroke=logo.small?32:24
            if((logo.artwork.pattern||"none")!=="none" || logo.artwork.texture){
                c.strokeStyle=logo.resolvedSurface;c.lineWidth=stroke+8
                c.beginPath();c.arc(128,128,84,start,end);c.stroke()
            }
            c.strokeStyle=logo.foregroundColor;c.lineWidth=stroke
            c.beginPath();c.arc(128,128,84,start,end);c.stroke()
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }
    onRetainedFractionChanged: if(ring)ring.requestPaint()
    onMeasuredChanged: if(ring)ring.requestPaint()
    onForegroundColorChanged: if(ring)ring.requestPaint()
    onResolvedSurfaceChanged: if(ring)ring.requestPaint()
    onArtworkChanged: if(ring)ring.requestPaint()
    onSmallChanged: if(ring)ring.requestPaint()
}

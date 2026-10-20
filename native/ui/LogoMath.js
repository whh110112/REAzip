.pragma library

function measured(ratio) { return typeof ratio === "number" && isFinite(ratio) && ratio >= 0 }
function retained(ratio) { return measured(ratio) ? Math.min(1, ratio) : 1 - 48 / 360 }
function composite(color, base) {
    return Qt.rgba(color.r * color.a + base.r * (1-color.a),
                   color.g * color.a + base.g * (1-color.a),
                   color.b * color.a + base.b * (1-color.a), 1)
}
function luminance(color) {
    function linear(x) { return x <= 0.04045 ? x / 12.92 : Math.pow((x+0.055)/1.055, 2.4) }
    return 0.2126*linear(color.r)+0.7152*linear(color.g)+0.0722*linear(color.b)
}
function contrast(a, b) {
    const x=luminance(a), y=luminance(b)
    return (Math.max(x,y)+0.05)/(Math.min(x,y)+0.05)
}
function foreground(accent, strong, text, surface, small) {
    const choices=[accent,strong,text], target=small?4.5:3
    for (const color of choices) {
        const flattened=composite(color,surface)
        if (contrast(flattened,surface)>=target) return flattened
    }
    const black=Qt.rgba(0,0,0,1), white=Qt.rgba(1,1,1,1)
    return contrast(black,surface)>=contrast(white,surface)?black:white
}

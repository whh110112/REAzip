# Themes and the adaptive A identity

[简体中文](themes.zh-CN.md)

The in-window A mark automatically follows the selected theme, including custom themes. There is no separate logo color, texture or ratio control. Switching themes or rescanning an updated theme updates its colors and background. Opening/switching archives updates the measured ring.

## Archive measurement

`retained = physical archive bytes / total original file bytes`; directories are excluded. A retained ratio of 60% leaves 60% of the annulus filled and 40% open (144°). Measured arcs have radial terminals so their area matches the fraction exactly. The round-ended A mark with its default 48° centerline opening is used when no complete measurement exists: no archive, loading, a failed refresh, empty archives or unavailable metadata.

Archive overhead can produce a retained ratio above 100%; the display saturates at a complete ring. The backend retains the real ratio. Filtering, selection and folder navigation never change it. Solid archives use physical size rather than incomplete per-entry packed sizes. Numbered `.001` volume sets include every volume; other multi-volume formats require an engine-provided total physical size, otherwise they retain the default mark. Sizes are not converted to 32-bit integers, and totals can exceed one unsigned 64-bit value.

The title bar, welcome area, archive details, extract dialog and About page share the same component. The window/taskbar icon, installer and OS file associations use the fixed default mark; opening a file never rewrites OS icons or registrations.

## Theme contract

Place JSON files in the theme directory opened from Settings → Appearance. Existing themes containing only `vars` continue to work. Custom themes inherit missing color tokens from `light` or `sleep`, according to `base`.

```json
{
  "id": "quiet-paper",
  "name": "静纸",
  "nameEn": "Quiet paper",
  "base": "light",
  "vars": {
    "--bg": "#FAF9F6",
    "--surface": "#F1F0EB",
    "--card": "#FFFFFF",
    "--text": "#292F2A",
    "--muted": "#59645C",
    "--accent": "#486451",
    "--accent-strong": "#334A3A"
  },
  "background": {
    "pattern": "dots",
    "opacity": 0.10,
    "spacing": 16,
    "texture": "textures/paper.png"
  }
}
```

`background` is optional. Patterns are `none`, `dots`, `lines` or `grid`; omitted values default to `none`, opacity `0.10` and spacing `16`. Opacity is 0–0.18; spacing is 8–64 logical pixels. Pattern colors come from the theme's muted token. A texture is optional and may be combined with a pattern. The content backdrop and mark both consume this metadata automatically. New pattern types can extend this object without adding logo settings.

Textures must be PNG, JPEG or WebP files inside the theme directory, at most 4 MiB and 4096×4096 pixels. Relative paths are resolved and checked before reading. Remote URLs, external paths and symlink escapes are ignored. A versioned local URL refreshes a changed texture on rescan. SVG/CSS/JavaScript is not executed. Unsupported or invalid background values fall back to the theme colors.

The mark uses `--accent`, then `--accent-strong`, then `--text`; if all have insufficient contrast, black or white is chosen automatically. Contrast is calculated in linear sRGB against the actual surface after alpha compositing: 3:1 for ordinary marks, 4.5:1 at 16–24 px. Textured backgrounds receive a narrow surface-colored underlay behind the ring for local legibility; the surrounding skin texture remains visible. Small marks use a thicker stroke, with the measured sector fraction unchanged.

## Verification and regeneration

Native GUI tests measure the rendered 60% annulus area, check unknown/empty/overhead cases, switch all six palettes, load a real custom texture and check low-contrast/alpha fallbacks. Controller tests calculate ratios from actual worker listings; engine tests verify the total size of real split volumes. Captures are CI artifacts, not README galleries.

`python native/tools/generate-logo.py` (Pillow) regenerates the fixed SVG, 1024px PNG and seven-size ICO. The generated ICO is reopened to verify its embedded dimensions. This does not affect the dynamic in-window geometry.

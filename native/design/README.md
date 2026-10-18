# Screenshot and visual evidence policy

The English and Chinese project READMEs each show one real application screenshot in the matching UI language. Keep previews small in number, label their captured version, and do not present old captures as proof of current acceptance.

- `native-reference/`: native application captures and their SHA-256 manifest. These are verification evidence, not a homepage gallery. Existing captures predate the settings layout fix; recapture before claiming visual acceptance.
- `reference/`: legacy design baselines used by migration and visual comparison. Keep them while acceptance tasks reference them.
- Root-level `shots-*.png`: obsolete Electron captures removed from the current tree. They remain available in Git history and the old release tags.
- New ad-hoc captures and full scale/theme/language matrices: put them under ignored `.native-build/` locally and attach review evidence to Actions artifacts or releases when needed. Only commit a deliberate baseline or the small set needed for documentation.

When replacing a README preview, capture the real application in the corresponding language, check it visually, update its manifest, and state the version. Do not translate or synthesize interface text in an image editor.

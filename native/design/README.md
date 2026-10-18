# Screenshot and visual evidence policy

The current source tree keeps one real preview per README language in `docs/screenshots/`, with its source/hash manifest. Capture the compiled application, inspect it, and record its version. Do not synthesize translated interface text.

Complete theme/language/scale matrices and tab-layout captures belong in ignored `.native-build/` output and Actions artifacts. Historical design/reference images remain in [Git history](https://github.com/whh110112/ZENzip/tree/601e6048824c0b36fa28267970833494f98a137d/native/design); they are not current interface evidence and are not shipped with current source packages.

`tokens.json` and `controls.json` retain the machine-readable design definitions. The current runtime palette is defined in `native/themes/builtin.json`.

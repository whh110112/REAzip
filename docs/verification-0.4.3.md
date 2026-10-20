# ZENzip 0.4.3 verification

This release adds task-scoped bulk conflict decisions and a manual stable-release update check in Settings → About.

## Acceptance scope

- Real worker extraction of a three-file ZIP with existing destination files: overwrite all, skip all, rename all, fresh-task reset, cancellation and invalid-choice rejection.
- Real GUI clicks through the controller and worker: overwrite all, remember skip, cancellation on a new task; 108 conflict layouts across six skins and 18 languages at the minimum window size.
- Existing 648 settings layouts and 108 archive-tab layouts; paired Chinese/English conflict and About captures remain build artifacts.
- Update checks: numeric stable-version ordering, latest/newer-local states, malformed responses, draft/prerelease rejection, foreign download-page rejection, network/TLS/timeout failures, HTTP errors, response-size limits and retry after failure. Opening About does not initiate a request.
- Stable publication consumes the new draft's creation response directly, avoiding the release-list visibility race observed during 0.4.2 publication. Existing source/package/hash gates remain required.

## Behavior and limits

“Do not ask again” applies only to remaining conflicts in the current task. It is not a global overwrite setting. Closing or cancelling the dialog cancels the task. “Overwrite all” immediately chooses overwrite for the current and remaining conflicts.

Update checks contact GitHub only when the user presses the button. Requests have a 15-second deadline and a 1 MiB response limit. Only stable numeric versions from this repository are accepted. Available updates open the matching official release page; downloads and installation remain explicit user actions. Offline or rate-limited requests show a retryable error.

The existing Windows 11 modern menu, drag-out storage and cross-application drag-and-drop limits remain unchanged. See [known limitations](../native/docs/known-limitations.md).

## Release gate

Publication requires all six OS/architecture targets and the aggregate candidate to pass on the exact main source. Uploaded package sizes and SHA256 values must match all six manifests before the draft becomes public. Build results and final release evidence are recorded only after verification completes.

# CoreFlow Material 3 WebUI test/integration fix

This patch aligns the WebUI contract tests with the current Material 3 dashboard DOM and removes HTML-string rendering from `app.js`. Dynamic content is now constructed with `document.createElement`, `appendChild`/`append`, and `textContent`.

## Included files
- `module/webroot/app.js`: safe DOM rendering for resources, decisions, system details, policy details, and journal entries.
- `module/webroot/index.html`: accurate CDN status wording; native controls remain usable with the local fallback.
- `tools/test_webui_status.js`: checks the current runtime renderer rather than the removed legacy render marker.
- `tools/test_webui_render.js`: checks actual Material 3 page IDs, handler IDs, API contracts, fallback wiring, and safe DOM construction.

## Validation
Run `sh tools/test_webui_status.sh`. Verified in the supplied source tree: status contract 200/200, Material 3 render contract 68/68, control contract 69/69. This is a host-side contract test, not a substitute for testing the WebUI on-device through KernelSU.

Observe remains the default. No new direct `/proc` or `/sys` writes are introduced; policy changes continue through existing `CoreFlowControl` validation and safety gates.

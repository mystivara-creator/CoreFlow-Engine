# WebUI journal and diagnostics audit

Base: `fb7cfdc` (the same source tree as local checkpoint `6ddb565`).
Scope: Journal & Logcat, diagnostic downloads, native synchronization and the
boundary between live daemon data and browser preview.

## Findings and resulting behavior

| Finding | Result |
| --- | --- |
| Export buttons relied on browser Blob navigation, without a KernelSU file-saving path or feedback. | Native exports stage bounded UTF-8 content privately and save it to `/sdcard/Download`; the UI reports the destination or the actual write error. Existing files are not overwritten. Browser exports retain their Blob URL long enough for download handling. |
| Reload hid shell failures and reread configuration alongside journal data. | Dedicated journal/log refresh exposes errors, a busy state and last-success time. It leaves unsaved settings intact. Polls share an in-flight request rather than racing. Journal polling runs only while the section is open. |
| The “Daemon Logcat” source was only supervisor stdout/stderr; daemon messages use Android logging. | Read tagged Android logcat and clearly separate supervisor output. Reads are bounded, and logcat failures retain their nonzero exit status. No system-wide log buffers are cleared. |
| Empty native logs fell back to simulated records. | Native empty states remain empty and unavailable sensors remain unavailable. |
| Filter buttons sent singular keys while the helper recognized only legacy plural keys. | Each visible filter now selects its intended events; failed restoration is visible in both Safety / Rollbacks and Errors. Verified partial writes remain discoverable even if the overall operation failed. |
| Journal rationale and values were truncated at short display limits. | Preserve substantially longer diagnostic fields; JSON export follows the selected filter/search. Mobile cards expose target, requested, before and factory values in expandable details. |
| Wide journal tables obscured columns on phones. | Phone layouts use wrapped cards; desktop retains its scrollable table. Action buttons and stream tabs wrap within the available width. |
| CSV mixed actual samples with preview initialization, fabricated battery electrical readings and a fixed RAM size; history was only six records and could duplicate a sample. | Native CSV contains up to 300 measured snapshots per WebUI session, with device timestamps, PID and sample identifiers. Missing fields stay blank. Repeated samples are replaced and daemon restarts separate sessions. CSV escaping covers Unicode, quotes, newlines and formula-like text. |
| Native Sysfs Inspector could fall back to preview mutations when no real changes were held. Governor experiments and forecast vectors were also simulated in the live view. | Native pages show daemon-supplied state only. Browser-only experiments and illustrative topology/forecast information stay in preview. The live console uses normalized daemon telemetry and measured time-series snapshots; unavailable values produce chart gaps, without illustrative guard thresholds or inferred readings. |
| Configuration read failure left default settings available for a save. | A successful current-config read is required before native settings can be saved; failures explain how to retry. |
| Malformed bridge callback exit codes could be interpreted as success. | Validate numeric/string exit codes and reject malformed values. |
| Procfs integer arrays contain tab separators, while the durable journal rejects tabs as field delimiters. | Canonicalize resource-value token separators to spaces before comparison/journaling. Keep the journal's strict delimiter rejection, checksums, pre-write durability and restoration gates. |

The screenshot's `JOURNAL_COMMIT_FAILED` is a real daemon-side failure, separate
from the export-button defects. A representative `1110000\t0\t0  0` vector failed
normalization/journal commit on the old code and passed on this change. This
reproduces one concrete cause; storage, permission or durability failures on the
phone still require its exported logs and must continue to block writes.

## Verification

- Production frontend build: TypeScript plus Vite passed; packaged public helpers
  match their source files. Production assets were regenerated.
- All 45 host core sources compiled with the project's strict C++20 warning flags
  and `-Werror`; all 25 existing host test executables passed. CMake is unavailable
  in this environment, so the source platform classification and host definitions
  were reproduced with direct G++ compilation.
- WebUI contracts: 224 status, 14 render and 70 control checks passed.
- Diagnostics: 43 checks exercise actual TypeScript bridge functions, React action
  handlers and App callbacks using a hook harness and sandboxed shell transport.
  This includes multichunk Unicode round trips, non-overwrite behavior, write/read
  failures, staging cleanup, search-aware exports, CSV accuracy, history bounds,
  preservation of unsaved settings and prevention of simulated native fallback.
- Safety foundation, source-release validation and 11 installer scenarios passed.
- Resource regression checks cover tab-separated vector normalization and durable
  journal round trips. No protection was relaxed to make journal commits succeed.

The handler harness is not a Chromium/WebView rendering test: no Chromium binary
is installed here. No Android NDK build, on-phone KernelSU/KernelSU Next test or
visual certification of the phone layout is claimed by these checks.

## Phone acceptance after installing the exact CI build

1. Open Journal & Logcat and press Reload. Confirm the timestamp advances or a
   visible read error appears; changing streams must never show preview records.
2. Export the trace and tagged logcat, then download CSV. Confirm each success
   message names a real file in Download and open those files in a file manager.
3. Apply each event filter and search text, then export again; contents must match
   the selected view. Check the mobile target/value details without horizontal
   page scrolling.
4. Edit an engine setting without saving, visit the journal, press Reload and
   return. The edit must remain. Explicit Sync may reload the on-disk settings.
5. Verify native CSV contains actual daemon sample IDs and timestamps, with blank
   missing readings. Restarting the daemon must not merge different sessions.
6. If journal commit failures persist, collect the new exports. A commit failure
   must still prevent the actuator write; do not delete recovery journals to hide it.

# ADR 0004: Durable mutation journal v2

- Status: Accepted
- Date: 2026-10-17

## Context

The mutation journal is the only durable evidence for restoring baseline values after an interrupted write. A missing terminator was detected in v1, but valid-looking payload corruption had no integrity check and the loader could allocate memory proportional to an untrusted file size. Recovery must also work across a package upgrade when a v1 journal remains from an earlier daemon.

## Decision

- New commits use journal v2 with a CRC32 checksum over the header and all entry records. The temporary file is uniquely named and created exclusively (`mkstemp`); a predictable temp path is not opened with truncation, because `O_NOFOLLOW` by itself does not reject hard links.
- The reader continues to accept well-formed v1 records for backward-compatible crash recovery, but it never rewrites or interprets a malformed v1 file as absent.
- Both formats require a final `end` marker, unique paths, non-empty fields, and no embedded NUL or record delimiter bytes.
- Journal input is bounded to 256 entries, 4096 bytes per field, and 1 MiB total; reads use a no-follow descriptor and reject non-regular files.
- A corrupt, oversized, symlinked, or truncated journal blocks autonomous mutation. Recovery remains explicit and must succeed before a new baseline is adopted.
- Resource writes use a no-follow descriptor checked via `fstat`, require a discovered kernel-tunable filesystem class (procfs, sysfs, cgroup v1/v2), and are followed by a live read-back. No production `ftruncate` is performed against kernel pseudo-filesystems.

## Consequences

- CRC32 detects accidental corruption and torn/modified payloads; it is not authentication against an actor able to rewrite the journal and recompute its checksum.
- Existing v1 journals remain recoverable after upgrading to v2.3.0.
- Journal size limits are part of the persistence contract; expanding them requires a deliberate change and tests.
- Host fixtures use ordinary files and perform a test-only truncate after validating the already-open descriptor to model sysfs attribute writes.

## Verification

Host tests cover v2 round-trip and checksum corruption, v1 compatibility, missing terminators, embedded NUL rejection, symlink rejection, size bounds, hard-link safety for the prior fixed temp name, directory-fsync errors, and fail-closed recovery without a journal. Android NDK and device recovery still require validation for the exact release candidate.

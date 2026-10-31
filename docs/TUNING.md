# Tuning Documentation

Document every user-visible or safety-relevant tuning behavior here.

For each tunable, record:

- Purpose and expected effect.
- Valid range and default value.
- Conditions under which CoreFlow may change it.
- Safety limits enforced by `src/conscience`.
- Rollback or recovery behavior.
- Device or chipset-specific caveats.

Keep tuning policies explicit. Avoid undocumented automatic behavior that could reduce stability, battery life, or thermal safety.

## v3.1.0 settings workflow

The WebUI provides per-field purpose, default, units, valid range and tradeoffs in
Indonesian. A settings choice changes a draft only. Review → Save writes config;
Restart applies it. Discard reverts the draft, not the device. The quieter choice
uses an 8-second monitor interval and 70% minimum confidence and disables temporary
interactivity assistance; the main optimizer remains active. Faster observation
uses 2 seconds and 65% confidence, without increasing boost amplitudes. Neither is
an FPS/thermal guarantee. Existing authority, capability, journaling and recovery
gates remain mandatory. See [checkpoint scope](CHECKPOINT_3.1.0.md).

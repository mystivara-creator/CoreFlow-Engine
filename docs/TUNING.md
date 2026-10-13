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

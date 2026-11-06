# CoreFlow CI and GitHub Repository Settings

This document describes the repository-side CI contract and the one-time GitHub settings needed to protect `main`.

## CI contract

The `CoreFlow Engine CI` workflow runs for every pull request targeting `main`, pushes, and manual dispatches. It separates WebUI, Rust, module lifecycle, host C++, sanitizers, and Android ARM64 packaging into independent jobs. A failure in one validation job does not prevent the others from running.

The stable required status check is **CoreFlow CI gate**. The gate succeeds only when all six validation jobs succeed:

- WebUI validation
- Rust validation
- Module validation
- Host C++ tests
- ASan and UBSan
- Android ARM64 production package

CTest logs are uploaded as workflow artifacts even when host tests fail. The workflow does not use `continue-on-error` to conceal test failures.

## Release safety

The CI workflow has read-only repository permissions and never creates or edits GitHub Releases. The separate `CoreFlow Release Publisher` workflow is triggered only after a successful CI run for a semantic version tag (`vMAJOR.MINOR.PATCH`). It downloads the package artifact from that exact successful run, verifies the ZIP and embedded `module.prop` version, and requires a matching changelog section before publishing.

The existing KernelSU update-feed workflow remains a downstream release-published action.

## One-time GitHub Settings

These settings require repository administration access and must be applied in the GitHub web interface:

1. Open **Settings → Actions → General**.
2. Under **Workflow permissions**, choose **Read repository contents permission** as the default.
3. Keep the option allowing GitHub Actions to create and approve pull requests disabled unless a future workflow explicitly requires it.
4. Open **Settings → Rules → Rulesets → New ruleset → New branch ruleset**.
5. Name it `CoreFlow Main Protection`, set enforcement to **Active**, and target the `main` branch.
6. Enable **Require a pull request before merging**. For a single-maintainer repository, required approvals can remain at zero.
7. Enable **Require status checks to pass** and add exactly **CoreFlow CI gate** after the first run of this workflow has reported that check.
8. Enable **Block force pushes** and **Restrict deletions**.
9. Do not require signed commits or additional approvals unless the maintainer intentionally adopts those policies.

Do not mark individual job checks as required in addition to the gate. The gate is the stable aggregate contract and avoids accidental merge blockage when internal job names are reorganized.

## Current known product test failure

The CI infrastructure must not weaken or skip product tests to obtain a green result. The observed run for commit `eaed149fa81cae26007bc245c59bb7d26314e2fd` compiled the host targets but failed `AutonomousTunerTest` at `tests/test_autonomous_tuner.cpp:242`, where the selected candidate did not equal `io.mq_deadline_low_latency`. This is a product/test behavior mismatch that must be diagnosed separately. Sanitizer and Android results from that old run were skipped, not passed.

## Maintainer rule

A green CI gate is necessary but not sufficient for a release. Review the diff, inspect failed-job artifacts, and validate runtime-sensitive behavior on supported Android hardware before publishing or flashing.

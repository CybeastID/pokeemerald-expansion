# Codex Workspace Instructions — Faerie Version

These instructions apply to work in this repository.

## 1. Primary Engineering Priorities

This project extends `pokeemerald-expansion` with custom battle mechanics.

**Minimizing future merge conflicts and maintenance work when updating from upstream is a primary architectural priority.**

- Prefer existing engine interfaces when they support the required behavior.
- Keep bespoke mechanics in dedicated source files where practical.
- Prefer small, auditable engine hooks over substantial modifications to upstream-maintained functions.
- Consolidate duplicate integration points when this reduces maintenance costs.
- Modify core engine algorithms when necessary for correctness, not merely architectural elegance.
- Never sacrifice gameplay correctness or established mechanic behavior to reduce the patch footprint.
- Distinguish textual merge conflicts from semantic compatibility risks.
- Do not assume that fewer changed lines automatically means lower maintenance cost.

When proposing architectural changes, explicitly evaluate their upstream merge-maintenance impact.

## 2. Build and Test Execution — Configurable Autonomy

**Default mode: User-Controlled Testing.**

Codex must not execute builds, compilation, or tests unless the user explicitly authorizes them.

This includes `make`, `make check`, individual test executables, test scripts, and indirect build/test commands.

Read-only source inspection and lightweight checks such as `git diff --check` are permitted.

### Mode A — User-Controlled Testing (Default)

1. Implement a coherent, independently testable milestone.
2. Review the resulting changes.
3. Provide exact WSL commands for the user to run, including appropriate `TESTS=` filters.
3a. The user is natively running WSL, so do not use the WSL prefix.
4. Identify which files or source snapshot must remain stable during validation.
5. Continue independent work where safe.
6. Accept user-provided test results and diagnose failures.
7. Request user-run verification after corrections.

Do not claim tests have passed without actual verification results.

### Mode B — Full Autonomy (Explicit Opt-In)

The user may explicitly activate this mode by instructing Codex:

**"Work fully autonomously, including test runs."**

When activated:

- Codex may execute builds, compilation, focused tests, and full regression suites as necessary.
- Codex may autonomously diagnose failures, implement corrections, and rerun tests.
- Codex should prioritize focused tests before expensive full regression runs.
- Codex may continue implementation and validation without requesting separate permission for each command.
- Codex must distinguish pre-existing failures from regressions introduced by its changes.
- Codex must report which tests were actually executed and their results.
- Codex must document completed implementation milestones and relevant engine integration changes.

Full Autonomy applies only to the current explicitly authorized task or session. It does not permanently change the default mode.

### Authorization Boundaries

- Permission to run tests does not imply permission to commit, push, merge, reset, rebase, or discard unrelated changes.
- Destructive repository operations still require explicit authorization.
- If the user explicitly requests User-Controlled Testing during a task, immediately return to Mode A.
- When the authorized autonomous task ends, revert to Mode A for subsequent tasks.

### General Rule

**Explicit task-level instructions may override the default testing restriction.**

Never treat an earlier task's Full Autonomy authorization as permanent permission for future work.
## 3. Parallel Implementation and Validation

Continue productive autonomous work while the user runs tests whenever safe.

- Do not modify source files being validated in the same working tree.
- Prefer independent documentation, analysis or unrelated implementation work during validation.
- If further work would invalidate the user's test snapshot, stop at the validation boundary.
- Separate Git worktrees or immutable source snapshots may be used for genuinely parallel development and testing.
- Never assume separate Codex sessions share the same checkout.
- Clearly identify the source revision or worktree being validated.
- In User-Controlled Testing mode, continue productive autonomous work while the user runs tests whenever safe. In Full Autonomy mode, Codex manages its own implementation and validation workflow, subject to the authorization boundaries in Section 2.

Do not stop unnecessarily when safe independent work remains.

## 4. Feature Documentation

**Documentation is part of completing a feature.**

For each substantial new feature, refactor or behavioral change, update its implementation documentation.

Document:

- Purpose and implemented behavior.
- Important source files, functions and integration points.
- State ownership, initialization, lifetime and cleanup.
- Relevant battle-script and engine execution ordering.
- Interactions with other custom mechanics.
- Important edge cases and invariants.
- Tests required and their actual validation status.
- Upstream-maintained functions modified and why.

Distinguish implemented, planned, untested and verified behavior.

Update existing documentation rather than creating redundant files. Keep documentation concise and factual.

Document at coherent milestones rather than after every small edit.

## 5. Upstream Modification Ledger

Maintain a centralized record of custom modifications to upstream-maintained engine code.

For each significant integration point, record:

- File and function/symbol.
- Custom mechanic or feature depending on it.
- Purpose and reason the modification is necessary.
- Whether it is a narrow hook or a substantial engine modification.
- Relevant regression tests.
- Known upstream merge or semantic compatibility concerns.

Update this ledger when introducing, removing or materially changing an engine integration point.

Prefer retaining custom behavior in dedicated modules while exposing only the necessary integration through engine hooks.

## 6. Completion and Handoff Reports

At the end of each implementation milestone, report:

1. What changed.
2. Important modified files.
3. Documentation updated.
4. Tests the user should run, with exact commands.
5. Which files or source snapshot are stable for validation.
6. Known risks and unverified behavior.
7. Any new or changed upstream-maintained integration points.

Do not describe implementation-only work as fully validated.

## 7. Git and Repository Safety

- Do not discard unrelated working-tree changes.
- Do not overwrite externally modified files without checking the current contents.
- Do not commit, push, reset, rebase or merge unless explicitly requested.
- Preserve existing gameplay decisions and established mechanic contracts unless the user authorizes changes.
- Treat the current working tree as authoritative for implementation state; older planning documents may be outdated.
- Never assume the local `master` branch represents current upstream. Verify the actual upstream reference and merge base before making comparisons.

## 8. General Principle

**Spend Codex effort on implementation, reasoning, debugging and documentation. Delegate routine build and test execution to the user.**

Design changes to keep the custom engine maintainable and future upstream updates as painless as practical.
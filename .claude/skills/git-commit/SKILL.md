---
name: git-commit
description: Commit and branch conventions for this project. Use whenever creating a commit, splitting work into commits, writing a commit message, or creating a branch.
---

# Committing

## Before committing

1. `git add -N <new files>` so they are visible to the formatter, then `git clang-format`.
2. The Definition of Done build step passes (`cmake --workflow --preset <dev preset>`).
3. Stage only the files for this concern: `git add <paths>` (never `git add -A` blindly).

If the work mixes concerns, split it into several commits, and check that **each** commit builds on its
own: `git stash --keep-index`, build, `git stash pop`.

## Message

```
<prefix>: short imperative summary in lowercase

Optional body: why the change was made, anything surprising.
Markdown is allowed.
```

- Subject at most 72 characters, no trailing period, describes the changed behavior
  ("fix: reject empty names in registry", not "fix: bug in registry").
- Explanations go in the body, not the subject. No ticket notes in the subject.

| Prefix   | Use for |
|----------|---------|
| `fix:`   | bug fixes, regressions, leaks, warnings, behavior corrections |
| `dev:`   | new features, new behavior, new capabilities |
| `ref:`   | reorganizing code without intending a behavior change |
| `infra:` | build system, dependencies, tooling, project structure |
| `test:`  | test-only changes |
| `ui:`    | user-interface-focused changes |
| `doc:`   | documentation only |
| `wip:`   | **never used by the AI.** Human emergency commits only; never satisfies the Definition of Done |

Choosing:
- correcting behavior → `fix:` (even when the bug came from an earlier `dev:` or `ref:` commit)
- adding behavior → `dev:`
- changing how the project is built, organized or tooled → `infra:`
- reshaping code without a behavior change → `ref:`
- Do not mix prefixes in one commit unless one change is required by the other; then use the prefix of the main change.

## Branches

Create a branch when the task needs more than one commit, or will span more than one session.
Single-commit tasks may be committed on the current branch.

Name: `<prefix>_<short_snake_case_title>`, with prefix `fix_`, `dev_`, `ref_`, `infra_`, `test_`, `ui_` or `doc_`.
Example: `dev_names_registry_export`.

## Never

- push, merge into `main`, or force-push
- amend or rebase commits that already exist on a remote

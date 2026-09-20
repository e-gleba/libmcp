---
name: libmcp-code-review
description: Review changes in this repository for correctness, regressions, CMake portability, unsafe scope growth, and missing verification. Use for pull-request reviews, working-tree reviews, and pre-merge checks.
---

## Review workflow

1. Read root `AGENTS.md` and inspect repository status.
2. Read changed files plus minimal surrounding code required to understand behavior.
3. Check correctness before style. Do not request unrelated cleanup.
4. For CMake changes, inspect target propagation, preset consistency, native versus cross behavior, test discovery, install/package effects, and generated-file paths.
5. For Python tooling, check path containment, command construction, bounded output/input, dependency isolation, and stdio cleanliness.
6. Verify claims using focused build, test, lint, or syntax commands when available.
7. Report findings by severity with file and line. Explain impact and smallest fix.
8. If no actionable findings exist, say so and list verification gaps.

## Finding bar

Report only defects, security risks, regressions, broken portability, or meaningful maintainability hazards introduced by the change. Do not report personal preferences or pre-existing issues outside changed scope.

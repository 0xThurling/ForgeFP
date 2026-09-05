# ForgeFP — Missing Features Roadmap

A feature-by-module backlog of what ForgeFP is missing, and **why each feature
belongs**. Use this as the implementation checklist. Each module file now
reflects reality: implemented features are listed as done, and only the
**remaining** backlog carries full **Why** + **Implementation** entries.

## How to use this folder

- Implement in `src/fp/<module>.hpp`, then **mirror the change** to
  `include/forgefp/fp/<module>.hpp` (the tree CMake installs). The two trees
  must stay byte-identical — see `meta.md` for the sync workflow.
- Update `README.md` when a feature lands (the README is the API contract).
- Add a test for every feature before marking it done (see `testing.md` —
  Forge's test integration is GoogleTest-based, enabled via
  `testing = true` + a `googletest` direct dependency; tests live in
  `test/` and run with `forge test` / `ctest`).
- When a feature lands, remove its entry from the module file and add it to
  the "Implemented" table.

## Priority legend

| Tag | Meaning |
|---|---|
| P0 | Users will hit the lack of this within the first hour of using the library. |
| P1 | Standard FP-library surface; its absence makes the module feel incomplete. |
| P2 | Nice-to-have; rounds out the story once P0/P1 land. |

## Status per module (as of last audit)

| File | Backlog status |
|---|---|
| [maybe.md](maybe.md) | **Done** — only actionable item: fix the `collect` bug (never pushes values) |
| [either.md](either.md) | 4 left: `bimap`, `swap`, `expect`, `ok_or` (P1/P2) |
| [result.md](result.md) | 5 left: `try_`, `traverse`, `combine2`, `context` (P1), `collect_all` (P2) |
| [validation.md](validation.md) | **Nothing landed yet** — `check`/`ensure`, `traverse` (P0), `combine`, `merge`, `to_result` (P1), helpers (P2) |
| [vec.md](vec.md) | **Nearly done** — 8 P2s + `zip3`/`zip_with3`, `unzip` (P1) |
| [ranges.md](ranges.md) | 6 left: `all/any/none/count`, `to_vector`, `flat_map`, `group_by` (P1), `zip/enumerate`, `fold_right/scan`, `chunk/windows`, `min/max` (P2) |
| [string.md](string.md) | 1 P1 (`to_int`/`to_double`) + 6 P2s |
| [combinator.md](combinator.md) | 6 left: `compose` variadic, `pipe_with` (P1), `fix`, `apply`, `when/unless`, `first/second` (P2) |
| [concurrent.md](concurrent.md) | **Nothing landed yet** — ThreadPool, pool par_map/par_for_each/par_reduce (P0/P1), Channel, actor Ask, `async_sequence`, `Async<T>` (P1), race/timeout/retry (P2) |
| [simd.md](simd.md) | **Nothing landed yet** — `reduce`/`dot`/`map_to`/span (P0/P1), masked tails, aliases, math, `par_map_inplace`, helpers (P1/P2) |
| [testing.md](testing.md) | No tests / no CI yet — P0 |
| [bench.md](bench.md) | Infra done (`bench/`, `scripts/run_bench.sh`, forge scripts); cases for unimplemented features commented |
| [meta.md](meta.md) | Header-sync guard script not yet created; README drift table outstanding |

## Cross-cutting gaps (read first)

1. **No tests exist** (`forge.lua` has `testing = false`, no `test/` dir).
   Everything in this folder is unguaranteed until `testing.md` is done.
2. **Two header trees** (`src/fp`, `include/forgefp/fp`) are hand-mirrored —
   currently in sync (last verified), but every feature listed here must land
   in both, or installed consumers get a different API than README users.
3. **README drift**: it claims `simd.hpp` is excluded from `all.hpp` (true in
   both trees now), documents `<fp/ranges.h>` (file is `ranges.hpp`), and
   describes `include/forgefp/fp/` as a 4-header "older subset" (it is a full
   copy). Fix in `meta.md`.
# maybe.hpp — implementation status

Current surface: `map`, `and_then`, `or_else(T)`, `filter`.

## Implemented (in `src/fp/maybe.hpp` — nothing left to add)

| Feature | Location |
|---|---|
| `or_else(fn)` — fallback function overload | `maybe.hpp` |
| `collect` — sequence of optionals | `maybe.hpp` |
| `apply` — applicative lift | `maybe.hpp` |
| `flatten` | `maybe.hpp` |
| `from_optional(msg)` | `result.hpp` |
| `to_optional` | `either.hpp` |
| `transpose` (both directions) | `result.hpp` |
| `value_or_lazy` | `maybe.hpp` |

## Actionable item (defect, not a new feature)

`collect` is implemented but **broken**: the loop never pushes values, so it
always returns an empty vector (`src/fp/maybe.hpp`):

- [ ] add the missing `out.push_back(*o);` inside the `collect` loop
- [ ] add a test: two present optionals → vector of both; one absent → nullopt

## Backlog

None — the module backlog is complete. Re-check with the README's §1
semantics documentation (map short-circuits, and_then fails through) when
adding tests (testing.md §2).
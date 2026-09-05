# either.hpp — implementation status

Current surface: `Either<E, T>` (`ok`, `err`, `is_ok`, `value`, `error`),
`map`, `and_then`, `or_else(fn)`, `map_error`, `flatten`, `to_optional`,
`lefts`, `rights`.

## Implemented (in `src/fp/either.hpp`)

| Feature | Status |
|---|---|
| `and_then` — monadic bind | done |
| `or_else(fn)` — recover with a function | done |
| `map_error` / `map_left` | done |
| `flatten` | done |
| `lefts` / `rights` | done |
| `to_optional` | done |

Note: `transpose` intentionally does **not** live here — it bridges to
`Result` and lives in `result.hpp` (a cyclic include would otherwise break
parse order).

## Backlog

## `bimap` [P1]

```
auto bimap(Either<E, T> const&, F on_err, G on_ok) -> Either<R1, R2>
```

**Why:** Applying `map_error` **and** `map` in one pass is the canonical
"complement both sides" combinator; it keeps pipelines from doing two passes
over a value that can only be one side at a time. Cheap to implement (two
branches), immediately useful when converting error + value types between
modules (e.g. `Result<std::string>` to `Result<uint16_t>` with a rewritten
message).

**Implementation**

```cpp
template <class E, class T, class F, class G>
auto bimap(Either<E, T> const& e, F on_err, G on_ok)
    -> Either<std::invoke_result_t<F, E>, std::invoke_result_t<G, T>> {
  using E2 = std::invoke_result_t<F, E>;
  using T2 = std::invoke_result_t<G, T>;
  if (e.is_ok())
    return Either<E2, T2>::ok(on_ok(e.value()));
  return Either<E2, T2>::err(on_err(e.error()));
}
```

## `swap` [P2]

```
Either<T, E> swap(Either<E, T> const&)
```

**Why:** Round-trips between functions that disagree on the "left" convention,
and unlocks generic code that works on `Either` without knowing which side is
which. P2: rarely needed, but trivial to add once the rest exists.

**Implementation**

```cpp
// Haskell semantics: swap(ok x) = Left x, swap(err e) = Right e —
// the old value becomes the new "error" side, the old error becomes the new "value" side.
template <class E, class T>
Either<T, E> swap(Either<E, T> const& e) {
  if (e.is_ok())
    return Either<T, E>::err(e.value());
  return Either<T, E>::ok(e.error());
}
```

## `expect` — checked getter [P2]

**Why:** `value()`/`error()` state in the README that misuse is UB; every user
will hit this in the debugger sooner or later. A `value_or_throw()` (or
`expect(msg)`) variant with a descriptive what() would convert silent UB into
a diagnosable failure. P2 because fixing the accessor contract is a design
decision, not a feature addition.

**Implementation** — needs `#include <stdexcept>`

```cpp
template <class E, class T>
T const& expect(Either<E, T> const& e, char const* msg) {
  if (!e.is_ok())
    throw std::runtime_error(msg);
  return e.value();
}
```

## `ok_or` — convert optional to Either [P2]

```
Either<E, T> ok_or(std::optional<T> const&, E error)
```

**Why:** Mirror of `from_optional` (result.md) for non-string error types.
P2 — most users only need the string version.

**Implementation** — needs `#include <optional>` (already present)

```cpp
template <class E, class T>
Either<E, T> ok_or(std::optional<T> const& o, E error) {
  return o ? Either<E, T>::ok(*o) : Either<E, T>::err(std::move(error));
}
```
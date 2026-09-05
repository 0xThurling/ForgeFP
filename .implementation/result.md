# result.hpp — implementation status

Current surface: `Result<T>`, `ok`, `err`, `sequence`, `from_optional`,
`transpose` (both directions). Legacy combinators (`and_then`, `or_else`,
`map_error`) live in `either.hpp` and work on `Result` unchanged.

## Implemented

| Feature | Location |
|---|---|
| `from_optional(msg)` | `result.hpp` |
| `transpose` (optional ↔ Result, both directions) | `result.hpp` |
| `or_else(fn)` / `recover` | `either.hpp` (documented in README §4) |
| `to_optional` | `either.hpp` |

## Backlog

## `try_` — exception-to-Result bridge [P1]

```
template <class F> auto try_(F f) -> Result<std::invoke_result_t<F>>
```

**Why:** The README's examples already show the manual `try { return ok(...);
} catch (...) { return err("..."); }` boilerplate. Exceptions and `Result` will
coexist in any real codebase; a one-call wrapper (`try_([] { return
std::stoi(s); })`) converts thrown `std::exception` messages into `err`
strings while keeping the happy path as a plain value.

**Implementation** — needs `#include <exception>`

```cpp
template <class F>
Result<std::invoke_result_t<F>> try_(F f) {
  using R = std::invoke_result_t<F>;
  try {
    return ok<R>(f());
  } catch (std::exception const& e) {
    return err<R>(e.what());
  } catch (...) {
    return err<R>("unknown exception");
  }
}
```

## `traverse` — map-and-sequence in one pass [P1]

```
Result<std::vector<R>> traverse(std::vector<T> const& v, F f)   // F: T -> Result<R>
```

**Why:** `sequence(map(v, f))` materializes an intermediate vector of Results
and duplicates the fail-fast check. `traverse` is the standard FP way to apply
a fallible function to a whole collection and stop at the first error — it is
the *reason* `sequence` exists. It slots directly into the `concurrent` world
(map every element, collect first failure).

**Implementation**

```cpp
template <class T, class F>
Result<std::vector<std::invoke_result_t<F, T>>> traverse(std::vector<T> const& v, F f) {
  using R = std::invoke_result_t<F, T>;
  std::vector<R> out;
  out.reserve(v.size());
  for (auto const& x : v) {
    auto r = f(x);
    if (!r.is_ok())
      return err<std::vector<R>>(r.error());
    out.push_back(r.value());
  }
  return ok(std::move(out));
}
```

## `combine2` — pair of Results [P1]

```
Result<std::pair<A, B>> combine2(Result<A> const&, Result<B> const&)
```

**Why:** Two fallible calls feeding one consumer (connect host + parse port +
read token) requires nested `and_then` or manual branching today. A tuple
combinator (Rust's monadic `zip`) collapses it in one step. `validation.hpp`
already ships its own `combine2`; `Result` users expect the same convenience,
with fail-fast semantics.

**Implementation** — needs `#include <utility>`

```cpp
template <class A, class B>
Result<std::pair<A, B>> combine2(Result<A> const& a, Result<B> const& b) {
  if (!a.is_ok())
    return err<std::pair<A, B>>(a.error());
  if (!b.is_ok())
    return err<std::pair<A, B>>(b.error());
  return ok<std::pair<A, B>>({a.value(), b.value()});
}
```

## `context` — annotate errors [P1]

```
Result<T> context(Result<T> const& r, std::string prefix)   // prefix + existing message
```

**Why:** Errors gain meaning only when they say *where* they happened.
"while reading config.json: not a number" beats "not a number" every time.
A `context` helper wraps `map_error` into the most common rewrite (string
concatenation), so users actually write annotated errors instead of raw
propagations.

**Implementation** — built on `map_error` (either.hpp)

```cpp
template <class T>
Result<T> context(Result<T> const& r, std::string const& prefix) {
  return map_error(r, [&](std::string const& e) { return prefix + e; });
}
```

## `collect_all` — accumulate all error messages [P2]

```
Result<std::vector<T>> collect_all(std::vector<Result<T>> const&)
```

**Why:** `sequence` fails fast; `validate_all` (validation.hpp) collects all
errors but changes the type. Between them sits the user who wants
"all failures, as one comma-joined string, but still a `Result`". P2 because
`validation.hpp` arguably owns this space — include only if the type juggling
story matters.

**Implementation**

```cpp
template <class T>
Result<std::vector<T>> collect_all(std::vector<Result<T>> const& rs) {
  std::vector<T> values;
  std::vector<std::string> errors;
  for (auto const& r : rs) {
    if (r.is_ok())
      values.push_back(r.value());
    else
      errors.push_back(r.error());
  }
  if (errors.empty())
    return ok(std::move(values));
  std::string joined;
  for (size_t i = 0; i < errors.size(); ++i)
    joined += (i ? "; " : "") + errors[i];
  return err<std::vector<T>>(joined);
}
```
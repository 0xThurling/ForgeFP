# validation.hpp — implementation status

Current surface: `Validation<T>`, `valid`, `invalid` (msg / list),
`validate_all` (sequence), `combine2`.

## Implemented

None of the backlog has landed yet (the header still matches the original
four helpers). Everything below is still to do.

## Backlog

## `check` / `ensure` — predicate to Validation [P0]

```
template <class T> Validation<T> ensure(bool ok, std::string msg, T value)  // attach value
template <class T, class Pred> Validation<T> check(Pred pred, std::string msg, T value)
```

**Why:** This module is the *entry point* for validation, yet nothing turns a
plain predicate/condition into a `Validation` without the user manually
branching `if (!cond) return invalid<T>(msg);` everywhere. `ensure(cond,
msg, value)` is the handiest form: "keep value if cond, else invalid". The
result is that `combine2`/`validate_all` — the only interesting combinators —
are unreachable ergonomically without check. This one function makes the
module usable from the first line.

**Implementation**

```cpp
// check(bool, msg) alone cannot produce a value of type T; the practical
// forms carry the value (or take the predicate):
template <class T>
Validation<T> ensure(bool ok, std::string msg, T value) {
  return ok ? valid<T>(std::move(value)) : invalid<T>(std::move(msg));
}

template <class T, class Pred>
Validation<T> check(Pred pred, std::string msg, T value) {
  return pred(value) ? valid<T>(std::move(value)) : invalid<T>(std::move(msg));
}
```

## `traverse` — validate a whole collection [P0]

```
Validation<std::vector<R>> traverse(std::vector<T> const& v, F f)   // F: T -> Validation<R>
```

**Why:** The entire point of `Validation` (vs `Result`) is **collecting all
failures**. The killer application is checking N items (fields of a form, rows
of a batch) and reporting every bad one. Today that requires
`validate_all(map(v, f))` with an intermediate materialization, or a hand loop.
`traverse` is the canonical combinator for exactly this and the module is
incomplete without it — it's the difference between "first error" and "all
errors" that this module exists to provide.

**Implementation**

```cpp
template <class T, class F>
Validation<std::vector<std::invoke_result_t<F, T>>> traverse(std::vector<T> const& v, F f) {
  using R = std::invoke_result_t<F, T>;
  std::vector<R> values;
  std::vector<std::string> errors;
  for (auto const& x : v) {
    auto r = f(x);
    if (r.is_ok()) {
      values.push_back(r.value());
    } else {
      auto const& msgs = r.error();
      errors.insert(errors.end(), msgs.begin(), msgs.end());
    }
  }
  if (!errors.empty())
    return invalid<std::vector<R>>(std::move(errors));
  return valid(std::move(values));
}
```

## `combine` (variadic) [P1]

```
template <class... Vs, class F> auto combine(F on_success, Vs... vs)
```

**Why:** `combine2` covers two fields; real forms have five to ten. Nested
`combine2(combine2(...))` is the exact boilerplate `combine` eliminates, and
it pairs with `curry`: `combine(curry(make_person), name, age, email)`.
Because the number of validation helpers grows per field count, a variadic
version is strictly more valuable than `combineN` one-by-one.

**Implementation** — returns `Validation<R>` with
`R = invoke_result_t<F, decltype(vs.value())...>`

```cpp
template <class F, class... Vs>
auto combine(F on_success, Vs const&... vs) {
  using R = std::invoke_result_t<F, decltype(vs.value())...>;
  std::vector<std::string> errors;
  auto collect = [&errors](auto const& v) {
    if (!v.is_ok()) {
      auto const& msgs = v.error();
      errors.insert(errors.end(), msgs.begin(), msgs.end());
    }
  };
  (collect(vs), ...);
  if (!errors.empty())
    return invalid<R>(std::move(errors));
  return valid<R>(on_success(vs.value()...));
}
```

Usage: `combine(curry(make_person), name, age, email)` — every error message
from every field is gathered.

## `merge` / semigroup for errors [P1]

```
Validation<T> merge(Validation<T> const& a, Validation<T> const& b)
// error sides concatenated; a's value wins if both are valid
```

**Why:** `combine2` needs both values to produce a result; sometimes you just
want to *join* two validations (e.g. a later check that refines an earlier
one). Merge with message concatenation gives Validation its monoid half and
composes with `traverse` to implement "validate step 1, then also step 2".

**Implementation**

```cpp
template <class T>
Validation<T> merge(Validation<T> const& a, Validation<T> const& b) {
  if (a.is_ok() && b.is_ok())
    return valid(a.value());
  std::vector<std::string> errors;
  for (auto const* v : {&a, &b})
    if (!v->is_ok()) {
      auto const& msgs = v->error();
      errors.insert(errors.end(), msgs.begin(), msgs.end());
    }
  return invalid<T>(std::move(errors));
}
```

Precedence note: when both sides are valid, `a`'s value wins — document this.

## `to_result` — collapse messages [P1]

```
Result<T> to_result(Validation<T> const&)   // first message (or joined) -> err
```

**Why:** Bridging the accumulating world to the fail-fast world lets one
function choose the policy per call site: validate with `Validation`, consume
with `Result`. Without it, the two modules cannot interoperate despite sharing
`Either` as storage.

**Implementation** — needs `#include "result.hpp"`

```cpp
template <class T>
Result<T> to_result(Validation<T> const& v) {
  if (v.is_ok())
    return ok(v.value());
  auto const& msgs = v.error();
  return err<T>(msgs.empty() ? "validation failed" : msgs.front());
}
```

## `validate_none` / `validate_any` helpers [P2]

**Why:** Predicates over collections (none empty, any present, all unique) are
the most common *custom* checks users write on top of `check`. Two tiny
collectors give idiomatic one-liners instead of manual loops feeding
`invalid`.

**Implementation**

```cpp
// one failure message per offending element — keeps the accumulate-all contract
template <class T, class Pred>
Validation<std::vector<T>> validate_none(std::vector<T> const& vs, Pred pred,
                                         std::string msg) {
  std::vector<std::string> errors;
  for (auto const& x : vs)
    if (pred(x))
      errors.push_back(msg);
  if (!errors.empty())
    return invalid<std::vector<T>>(std::move(errors));
  return valid(vs);
}

template <class T, class Pred>
Validation<std::vector<T>> validate_any(std::vector<T> const& vs, Pred pred,
                                        std::string msg) {
  for (auto const& x : vs)
    if (pred(x))
      return valid(vs);
  return invalid<std::vector<T>>(std::move(msg));
}
```
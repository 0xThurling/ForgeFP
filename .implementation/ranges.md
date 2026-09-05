# ranges.hpp — implementation status

Current surface: `filter_map`, `fold_left`, `map`, `filter`, `take`, `drop`,
`concat`.

## Implemented (in `src/fp/ranges.hpp`)

| Feature | Status |
|---|---|
| `map` / `filter` | done |
| `take` / `drop` | done |
| `concat` (range of ranges) | done |

## Backlog

## `all` / `any` / `none` / `count` [P1]

```
bool all(R&& r, F pred); bool any(R&& r, F pred); bool none(R&& r, F pred)
size_t count(R&& r, F pred)
```

**Why:** The predicate trio exists for vectors; the entire point of this
module is that views and spans should get the same treatment. `std::ranges`
has the algorithms (`all_of` etc.) but the fp-style one-argument-call
wrappers keep pipelines uniform. Also `count` is the natural partner of
`filter_map`'s keep-pattern for "how many parsed".

**Implementation** — plus a value overload of `count`

```cpp
template <std::ranges::range R, class F>
bool all(R&& r, F pred) { return std::ranges::all_of(r, pred); }

template <std::ranges::range R, class F>
bool any(R&& r, F pred) { return std::ranges::any_of(r, pred); }

template <std::ranges::range R, class F>
bool none(R&& r, F pred) { return std::ranges::none_of(r, pred); }

template <std::ranges::range R, class F>
size_t count(R&& r, F pred) { return std::ranges::count_if(r, pred); }

template <std::ranges::range R, class T>
size_t count(R&& r, T const& v) { return std::ranges::count(r, v); }
```

## `to_vector` [P1]

```
template <std::ranges::range R> auto to_vector(R&& r) -> std::vector<std::ranges::range_value_t<R>>
```

**Why:** Every combinator in this module returns vectors anyway, but users
hand-roll `std::vector(r.begin(), r.end())` for *their* views constantly —
including views produced inside `pipe` from `into()`. A one-liner `to_vector`
closes the loop: eagerly materialize any range when leaving the view world.
Trivial, and makes `ranges.hpp` usable as the "adapter to become a vector" API.

**Implementation**

```cpp
template <std::ranges::range R>
auto to_vector(R&& r) {
  using T = std::ranges::range_value_t<R>;
  std::vector<T> out;
  if constexpr (std::ranges::sized_range<R>)
    out.reserve(std::ranges::size(r));
  for (auto&& x : r)
    out.push_back(std::forward<decltype(x)>(x));
  return out;
}
```

## `flat_map` [P1]

```
auto flat_map(R&& r, F f) -> std::vector<R2>   // F: T -> range
```

**Why:** The generic counterpart of vec's `flat_map`: map-and-concatenate over
any source range, including views. Tokenizing, expansions, and
keep-if-nonempty patterns (each element maps to 0..n others) are the classic
generator idiom. `filter_map` covers 0-or-1; `flat_map` covers 0-to-many —
the two belong together (e.g. `flat_map(words, str::split)`).

**Implementation**

```cpp
template <std::ranges::range R, class F>
auto flat_map(R&& r, F f) {
  using Sub = std::invoke_result_t<F, std::ranges::range_value_t<R>>;
  static_assert(std::ranges::range<Sub>);
  using T = std::ranges::range_value_t<Sub>;
  std::vector<T> out;
  for (auto&& x : r)
    for (auto&& y : f(x))
      out.push_back(std::forward<decltype(y)>(y));
  return out;
}
```

## `group_by` [P1]

```
auto group_by(R&& r, F key_fn) -> std::unordered_map<K, std::vector<T>>
```

**Why:** Grouping is the highest-value "restructure" operation and currently
only exists for `vector`. Log lines, CSV rows, event streams — all arrive via
views. Same implementation as vec's, one template loosened; gives the range
module a "collector" story next to its "transformer" one.

**Implementation** — needs `#include <unordered_map>`

```cpp
template <std::ranges::range R, class F>
auto group_by(R&& r, F key_fn) {
  using T = std::ranges::range_value_t<R>;
  using K = std::invoke_result_t<F, T>;
  std::unordered_map<K, std::vector<T>> out;
  for (auto&& x : r)
    out[key_fn(x)].push_back(std::forward<decltype(x)>(x));
  return out;
}
```

## `zip` / `enumerate` [P2]

```
auto zip(R&& a, R2&& b) -> std::vector<std::pair<A, B>>
auto enumerate(R&& r) -> std::vector<std::pair<size_t, T>>
```

**Why:** Alignment of two views (timestamps × values, names × scores) is a
read-only pairing that shouldn't force materializing both sides first.
P2 — vec already covers the eager version; only worth it once the other
range combinators exist.

**Implementation**

```cpp
template <std::ranges::range R1, std::ranges::range R2>
auto zip(R1&& a, R2&& b) {
  using A = std::ranges::range_value_t<R1>;
  using B = std::ranges::range_value_t<R2>;
  std::vector<std::pair<A, B>> out;
  auto ia = std::ranges::begin(a), ib = std::ranges::begin(b);
  auto ea = std::ranges::end(a), eb = std::ranges::end(b);
  for (; ia != ea && ib != eb; ++ia, ++ib)
    out.emplace_back(*ia, *ib);
  return out;
}

template <std::ranges::range R>
auto enumerate(R&& r) {
  using T = std::ranges::range_value_t<R>;
  std::vector<std::pair<size_t, T>> out;
  size_t i = 0;
  for (auto&& x : r)
    out.emplace_back(i++, std::forward<decltype(x)>(x));
  return out;
}
```

## `fold_right` / `scan` [P2]

```
auto fold_right(R&& r, T init, F op)
auto scan(R&& r, T init, F op) -> std::vector<T>
```

**Why:** `fold_left` exists; `fold_right` completes the fold pair for
non-associative ops (string building, tree-algebra code). `scan` gives running
accumulations over ranges. P2: use is niche, but the fold symmetry is cheap.

**Implementation** — `fold_right` materializes once (ranges have no standard reverse-iterate)

```cpp
template <std::ranges::range R, class T, class F>
T fold_right(R&& r, T init, F op) {
  std::vector<std::ranges::range_value_t<R>> tmp(r.begin(), r.end());
  for (size_t i = tmp.size(); i-- > 0;)
    init = op(tmp[i], std::move(init));
  return init;
}

template <std::ranges::range R, class T, class F>
std::vector<T> scan(R&& r, T init, F op) {
  std::vector<T> out;
  for (auto&& x : r) {
    init = op(init, x);
    out.push_back(init);
  }
  return out;
}
```

## `chunk` / `windows` [P2]

```
auto chunk(R&& r, size_t n) -> std::vector<std::vector<T>>
auto windows(R&& r, size_t n) -> std::vector<std::vector<T>>   // overlapping
```

**Why:** Batch-processing a span in slices and computing rolling statistics
(moving average over a filter view) are the classic view-consumer patterns.
P2 — vec's `chunk` exists for vectors; generic support matters only when
sources are views.

**Implementation** — `chunk` of `n == 0` yields empty

```cpp
template <std::ranges::range R>
auto chunk(R&& r, size_t n) {
  using T = std::ranges::range_value_t<R>;
  std::vector<std::vector<T>> out;
  if (n == 0)
    return out;
  std::vector<T> cur;
  for (auto&& x : r) {
    cur.push_back(std::forward<decltype(x)>(x));
    if (cur.size() == n) {
      out.push_back(std::move(cur));
      cur.clear();
    }
  }
  if (!cur.empty())
    out.push_back(std::move(cur));
  return out;
}

template <std::ranges::range R>
auto windows(R&& r, size_t n) {
  using T = std::ranges::range_value_t<R>;
  std::vector<std::vector<T>> out;
  std::vector<T> buf;
  for (auto&& x : r) {
    buf.push_back(std::forward<decltype(x)>(x));
    if (buf.size() > n)
      buf.erase(buf.begin());
    if (buf.size() == n)
      out.push_back(buf);
  }
  return out;
}
```

## `minimum` / `maximum` [P2]

```
std::optional<T> minimum(R&& r); std::optional<T> maximum(R&& r)
```

**Why:** Needs no materialization and reads better than the fold form for the
single most common aggregation. P2 — `fold_left` can express it; this is
ergonomics.

**Implementation** — needs `#include <optional>`; empty ranges yield `nullopt`

```cpp
template <std::ranges::range R>
std::optional<std::ranges::range_value_t<R>> minimum(R&& r) {
  if (std::ranges::empty(r))
    return std::nullopt;
  return *std::ranges::min_element(r);
}

template <std::ranges::range R>
std::optional<std::ranges::range_value_t<R>> maximum(R&& r) {
  if (std::ranges::empty(r))
    return std::nullopt;
  return *std::ranges::max_element(r);
}
```
# vec.hpp — implementation status

Current surface: `map`, `filter`, `zip`, `zip_with`, `head`, `partition`,
`group_by`, `chunk`.

## Implemented (in `src/fp/vec.hpp`)

| Feature | Status |
|---|---|
| `take` / `drop` | done |
| `take_while` / `drop_while` | done |
| `tail` / `init` | done |
| `last` | done |
| `reverse` | done |
| `sort` / `sort_by` | done |
| `all` / `any` / `none` | done |
| `find` / `contains` / `count_if` | done |
| `concat` / `flatten` | done |
| `flat_map` | done |
| `enumerate` | done |
| `unique` | done (bonus — not in the original backlog) |

## Backlog

## `zip3` / `zip_with3` [P1]

```
std::vector<std::tuple<A, B, C>> zip3(std::vector<A> const&, std::vector<B> const&, std::vector<C> const&)
```

**Why:** RGB channels, time-series of (time, value, flag), and triplanar data
are exactly three-wise; binary `zip` can't express them without nesting pairs.
Trivial generalization of `zip`, and the natural companion users will reach
for immediately.

**Implementation** — needs `#include <tuple>`

```cpp
template <class A, class B, class C>
std::vector<std::tuple<A, B, C>> zip3(std::vector<A> const& a,
                                      std::vector<B> const& b,
                                      std::vector<C> const& c) {
  auto n = std::min({a.size(), b.size(), c.size()});
  std::vector<std::tuple<A, B, C>> out;
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
    out.emplace_back(a[i], b[i], c[i]);
  return out;
}

template <class A, class B, class C, class F>
auto zip_with3(std::vector<A> const& a, std::vector<B> const& b,
               std::vector<C> const& c, F f) {
  std::vector<std::invoke_result_t<F, A, B, C>> out;
  auto n = std::min({a.size(), b.size(), c.size()});
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
    out.push_back(f(a[i], b[i], c[i]));
  return out;
}
```

## `unzip` [P1]

```
std::pair<std::vector<A>, std::vector<B>> unzip(std::vector<std::pair<A, B>> const&)
```

**Why:** The exact inverse of `zip` — the module ships `zip` but not its
inverse, so round-tripping data requires a hand loop. Needed for separating
interleaved streams and for feeding `zip`-paired data into column-oriented
code.

**Implementation**

```cpp
template <class A, class B>
std::pair<std::vector<A>, std::vector<B>> unzip(std::vector<std::pair<A, B>> const& ps) {
  std::vector<A> as;
  std::vector<B> bs;
  as.reserve(ps.size());
  bs.reserve(ps.size());
  for (auto const& [a, b] : ps) {
    as.push_back(a);
    bs.push_back(b);
  }
  return {std::move(as), std::move(bs)};
}
```

## `sum` / `product` / `maximum` / `minimum` [P2]

```
T sum(std::vector<T> const& v)             // via fold
T product(std::vector<T> const& v)
std::optional<T> maximum(std::vector<T> const& v)  // empty-safe
std::optional<T> minimum(std::vector<T> const& v)
```

**Why:** Aggregate statistics are the most common fold applications and
`fold_left` currently lives in `ranges.hpp` with manual init/op plumbing.
Thin wrappers with empty-safe semantics (`optional` for min/max) give
one-word calls.

**Implementation** — needs `#include <numeric>` for `sum`/`product`

```cpp
template <class T>
T sum(std::vector<T> const& v) {
  return std::accumulate(v.begin(), v.end(), T{});
}

template <class T>
T product(std::vector<T> const& v) {
  return std::accumulate(v.begin(), v.end(), T{1});
}

template <class T>
std::optional<T> maximum(std::vector<T> const& v) {
  if (v.empty())
    return std::nullopt;
  return *std::max_element(v.begin(), v.end());
}

template <class T>
std::optional<T> minimum(std::vector<T> const& v) {
  if (v.empty())
    return std::nullopt;
  return *std::min_element(v.begin(), v.end());
}
```

## `span` / `break_` [P2]

```
std::pair<std::vector<T>, std::vector<T>> span(std::vector<T> const& v, F pred)
// keep prefix while pred holds; the rest follows
```

**Why:** Splitting a *prefix* (sorted runs, consecutive duplicates, header
lines) — `partition` splits by truthiness across the whole vector, but
positional splitting is a different, frequent need. (Note: `drop_while` is
done; `span` is the pair — prefix + remainder — and still missing.)

**Implementation** — unlike `partition`, the split is *positional* (prefix vs. suffix)

```cpp
template <class T, class F>
std::pair<std::vector<T>, std::vector<T>> span(std::vector<T> const& v, F pred) {
  auto it = std::find_if_not(v.begin(), v.end(), pred);
  return {std::vector<T>(v.begin(), it), std::vector<T>(it, v.end())};
}
```

## `scan` — running accumulation [P2]

```
std::vector<T> scan(std::vector<T> const& v, T init, F op)   // inclusive/exclusive variant
```

**Why:** Prefix sums (integrals, running totals, progress metrics) are the
"fold with history" operation. Rare enough for P2, but it's the standard
companion to `fold_left` and rounds out the reduce family.

**Implementation** — inclusive: output[i] = op(init, v[0..i])

```cpp
template <class T, class F>
std::vector<T> scan(std::vector<T> const& v, T init, F op) {
  std::vector<T> out;
  out.reserve(v.size());
  T acc = init;
  for (auto const& x : v) {
    acc = op(acc, x);
    out.push_back(acc);
  }
  return out;
}
```

## `intersperse` / `intercalate` [P2]

```
std::vector<T> intersperse(std::vector<T> const& v, T sep)
std::vector<T> intercalate(std::vector<std::vector<T>> const& vs, std::vector<T> sep)
```

**Why:** Join vectors with separators (the vector analogue of `str::join`,
which already exists for strings) — CSV assembly, run-sequence joins.
Pairs neatly with `str::join` for strings.

**Implementation**

```cpp
template <class T>
std::vector<T> intersperse(std::vector<T> const& v, T sep) {
  std::vector<T> out;
  out.reserve(v.size() * 2);
  for (size_t i = 0; i < v.size(); ++i) {
    if (i)
      out.push_back(sep);
    out.push_back(v[i]);
  }
  return out;
}

template <class T>
std::vector<T> intercalate(std::vector<std::vector<T>> const& vs,
                           std::vector<T> const& sep) {
  std::vector<T> out;
  for (size_t i = 0; i < vs.size(); ++i) {
    if (i)
      out.insert(out.end(), sep.begin(), sep.end());
    out.insert(out.end(), vs[i].begin(), vs[i].end());
  }
  return out;
}
```

## `replicate` / `range` [P2]

```
std::vector<T> replicate(size_t n, T x)
std::vector<int> range(int from, int to)     // [from, to), step overload
```

**Why:** Generating test data, seeding windows (`take(3, range(...))`), and
finite-sequence idioms (`replicate` is the vector side of `curry`'s
`const_`). Fills the "make a vector" half the module entirely lacks
(no constructors beyond `{}`).

**Implementation**

```cpp
template <class T>
std::vector<T> replicate(size_t n, T x) {
  return std::vector<T>(n, std::move(x));
}

template <class T = int>
std::vector<T> range(T from, T to, T step = 1) {
  std::vector<T> out;
  if (step == T{})
    return out;
  for (T i = from; i < to; i += step)
    out.push_back(i);
  return out;
}
```
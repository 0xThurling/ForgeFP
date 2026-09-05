# combinator.md — combinators.hpp / compose.hpp / curry.hpp — status

Current surface: `identity`, `const_`, `flip`, `on` (combinators.hpp);
`compose` (binary), `pipe`, `into`/`out`/`operator|`/`tap` (compose.hpp);
`curry`, `uncurry` (curry.hpp).

## Implemented

| Feature | Location |
|---|---|
| `uncurry` | `curry.hpp` |
| `on` | `combinators.hpp` |

## Backlog

## `compose` variadic (right-to-left) [P1]

```
template <class... Fs> auto compose(Fs... fs)   // compose(f, g, h) = f(g(h(x)))
```

**Why:** `pipe` is variadic left-to-right, but `compose` silently handles only
two functions. Mathematicians and point-free style read right-to-left; a
variadic `compose` is the mirror image one terminal-tab away, and the README
peddles `compose(f, g)(x) == f(g(x))` as a core feature with no way to
generalize. Cheap: reuse the same recursion `pipe` uses, reversed.

**Implementation** — mirror of `pipe`'s recursion, reversed; zero/one-arity fall through naturally

```cpp
template <class F>
auto compose(F f) {
  return f;
}

template <class F, class G, class... Fs>
auto compose(F f, G g, Fs... fs) {
  return [f = std::move(f), rest = compose(std::move(g), std::move(fs)...)](auto&&... args) {
    return f(rest(std::forward<decltype(args)>(args)...));
  };
}
```

## `fix` — Y combinator for self-reference [P2]

```
template <class F> auto fix(F f)   // f receives the recursive callable
```

**Why:** The README's fib example needs `std::function` plumbing purely so the
lambda can call itself. `fix` gives anonymous recursion without the
`std::function` arm-twisting:

```cpp
auto fib = fix([](auto self, int n) -> long long {
    return n < 2 ? n : self(n - 1) + self(n - 2);
});
fib(20);
```

It also composes with `memoize` (thread a self-call through the cache).
P2 — niche, but it removes a documented wart (`memoize.hpp`'s README note
about `std::function`).

**Implementation** — anonymous recursion without `std::function`

```cpp
template <class F>
auto fix(F f) {
  return [f = std::move(f)](auto&&... args) -> decltype(auto) {
    auto self = [&](auto&&... inner) -> decltype(auto) {
      return f(self, std::forward<decltype(inner)>(inner)...);
    };
    return self(std::forward<decltype(args)>(args)...);
  };
}
```

Usage:

```cpp
auto fib = fix([](auto self, int n) -> long long {
    return n < 2 ? n : self(n - 1) + self(n - 2);
});
fib(20);   // 6765
```

## `apply` — call a stored functional with args [P2]

```
auto apply(F f, Ts... ts) -> std::invoke_result_t<F, Ts...>   // = f(ts...)
```

**Why:** Uniform entry for "invoke whatever this is" in generic pipeline code
(and the classic way to feed a tuple's contents). Mostly aliases `std::apply`/
`std::invoke`; P2 — only helpful once `curry`/`uncurry`/`pipe` are in use
heavily.

**Implementation** — needs `#include <functional>`

```cpp
template <class F, class... Ts>
auto apply(F f, Ts... ts) -> std::invoke_result_t<F, Ts...> {
  return std::invoke(std::move(f), std::move(ts)...);
}
```

## `when` / `unless` — lazy conditional execution [P2]

```
auto when(bool cond, F f)        // run f only if cond; identity otherwise
auto unless(bool cond, F f)
```

**Why:** Debug hooks and optional post-processing in pipes:
`into(x) | when(debug, tap(log)) | ...`. This is the "pipe-friendly if" that
keeps pipelines declarative instead of collapsing into `if` blocks. (Naming
is contested — see open questions in the original combinator.md notes.)

**Implementation** — requires `f` to return the same type as its argument
(use with `tap` for side effects)

```cpp
template <class F>
auto when(bool cond, F f) {
  return [cond, f = std::move(f)](auto&& x) {
    return cond ? f(std::forward<decltype(x)>(x))
                : std::forward<decltype(x)>(x);
  };
}

template <class F>
auto unless(bool cond, F f) {
  return when(!cond, std::move(f));
}
```

Usage: `into(x) | when(debug, tap(log)) | ...`

## `first` / `second` — map over pair elements [P2]

```
auto first(F f)   // transform std::pair's first
auto second(F f)
```

**Why:** Pair/tuple transformation for code working with `zip`/`unzip`
(vec.md) and `group_by` outputs: `map(pairs, second(to_string))`.
Pairs well with `on`-style projections.

**Implementation**

```cpp
template <class F>
auto first(F f) {
  return [f = std::move(f)](auto p) {
    p.first = f(std::move(p.first));
    return p;
  };
}

template <class F>
auto second(F f) {
  return [f = std::move(f)](auto p) {
    p.second = f(std::move(p.second));
    return p;
  };
}
```

Usage with vec combinators: `map(pairs, second(str::to_string))`.

## `pipe_with` — lift variadic consumers into a pipe [P1]

**Why:** `into(x) | f` applies unary f. For variadic consumers
(`into(vector) | fold_left(0, +)`), a Piped helper that consumes additional
arguments makes the pipe style uniform with the eager ranges/vec API. P1 *if*
the Piped style is meant to be the library's flagship (README's top example
says it is).

**Implementation** — a helper, not an `operator|` overload (binary `|` can't
carry extra args cleanly); decide the name in the README

```cpp
// compose.hpp
template <class T, class F, class... Ts>
auto pipe_with(Piped<T> p, F f, Ts const&... ts) {
  return Piped<decltype(f(std::move(p.value), ts...))>{
      f(std::move(p.value), ts...)};
}
```

---

## Open design questions (resolve before implementing the P2s)

1. **`when`/`unless` naming** — collides with `std::ranges`-adjacent naming and
   `boost::hana` conventions. Choose, document, and keep.
2. **`fix` + mutable state** — `fix` returns a lambda; making it share state
   with `memoize` requires the memoized function to live in the same closure.
   Decide whether `fix` should compose with `memoize` officially.
3. **`compose` arity-zero/one** — should `compose()` = `identity` and
   `compose(f)` = `f`? Cheap to support, big ergonomics win for generic code
   building pipelines dynamically.
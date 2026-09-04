# ForgeFP

A header-only, C++20 functional programming library. ForgeFP brings typical functional programming tools to modern C++: algebraic data types (`Either`, `Result`, `Validation`), optional composition, vector/range combinators, function composition, currying, memoization, string utilities, SIMD mapping, and concurrency helpers — with zero external dependencies.

```cpp
#include <fp/all.hpp>
#include <iostream>

int main() {
    auto answer = fp::out(fp::into(3)
        | [](int x) { return x * x; }                     // 9
        | [](int x) { return x + 33; }                    // 42
        | fp::tap([](int x) { std::cout << x << "\n"; })); // prints 42
}
```

---

## Table of contents

- [Requirements](#requirements)
- [Getting the headers](#getting-the-headers)
- [Building / installing](#building--installing)
- [API reference](#api-reference)
  - [1. `maybe.hpp` — optional composition](#1-maybehpp--optional-composition)
  - [2. `adt.hpp` — pattern matching over variants](#2-adthpp--pattern-matching-over-variants)
  - [3. `either.hpp` — sum types](#3-eitherhpp--sum-types)
  - [4. `result.hpp` — string-typed errors](#4-resulthpp--string-typed-errors)
  - [5. `validation.hpp` — accumulating validation](#5-validationhpp--accumulating-validation)
  - [6. `vec.hpp` — vector combinators](#6-vechpp--vector-combinators)
  - [7. `ranges.h` — range combinators](#7-rangesh--range-combinators)
  - [8. `string.hpp` — string utilities](#8-stringhpp--string-utilities)
  - [9. `combinators.hpp` — identity, const, flip](#9-combinatorshpp--identity-const-flip)
  - [10. `compose.hpp` — function composition and pipes](#10-composehpp--function-composition-and-pipes)
  - [11. `curry.hpp` — partial application](#11-curryhpp--partial-application)
  - [12. `memoize.hpp` — cached functions](#12-memoizehpp--cached-functions)
  - [13. `concurrent.hpp` — parallel map, futures, actors](#13-concurrenthpp--parallel-map-futures-actors)
  - [14. `simd.hpp` — SIMD-accelerated mapping](#14-simdhpp--simd-accelerated-mapping)
- [Limitations and notes](#limitations-and-notes)

---

## Requirements

| Requirement | Version |
|---|---|
| C++ standard | C++20 |
| Compiler | GCC 11+ or Clang 14+ (libstdc++ recommended) |
| Threading | `-pthread` when using `concurrent.hpp` |
| Architecture | `simd.hpp` requires x86-64 / ARM64 (or any target with `native_simd` support) |

The library is **header-only**, so no compilation step is required for the library itself.

---

## Getting the headers

Headers live in two places:

- **`src/fp/`** — the full library, one header per module. Include as `#include <fp/all.hpp>` (umbrella header) or a single module (`#include <fp/vec.hpp>`).
- **`include/forgefp/fp/`** — the installed/public subset (`adt`, `either`, `maybe`, `result`). Include as `#include <forgefp/fp/result.hpp>`.

Quick start with the full library:

```bash
g++ -std=c++20 -I <path-to-forgefp>/src my_program.cpp -pthread
```

```cpp
#include <fp/all.hpp>            // everything except ranges
#include <fp/ranges.h>           // newest module, include manually

int main() {
    std::vector<int> v{1, 2, 3};
    auto doubled = fp::map(v, [](int x) { return x * 2; });
}
```

> The namespace is `fp` (string helpers are in `fp::str`).

---

## Building / installing

This is a [Forge](https://forge) project; the `CMakeLists.txt` is generated (`forge build` regenerates it, but you can also use CMake directly).

```bash
# Forge
forge build

# Or plain CMake
cmake -S . -B build && cmake --build build
```

Because there are no `.cpp` files in `src/`, ForgeFP is installed as a header-only **INTERFACE** library:

```cmake
# consumer CMake
find_package(forgefp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE forgefp)
```

or copy `src/fp/` into your project and add its parent directory to your include path.

---

## API reference

All examples assume `using namespace fp;` unless noted.

### 1. `maybe.hpp` — optional composition

Combinators over `std::optional<T>` that let you chain operations without nested `if` checks.

| Function | Signature | Description |
|---|---|---|
| `map` | `(std::optional<T>, F) -> std::optional<invoke_result_t<F, T>>` | Apply `f` if a value is present; otherwise propagate `nullopt`. |
| `and_then` | `(std::optional<T>, F) -> invoke_result_t<F, T>` | Apply a *monadic* function `f` that itself returns an optional. |
| `or_else` | `(std::optional<T>, T) -> T` | Unwrap with a fallback value. |
| `filter` | `(std::optional<T>, F) -> std::optional<T>` | Keep the value only if a predicate is true. |

```cpp
std::optional<int> parse(const std::string& s);   // some external parser

auto result = and_then(filter(map(parse("42"), [](int x) { return x + 1; }),
                               [](int x) { return x > 0; }),
                        [](int x) { return std::optional<int>(x * 2); });

int n = or_else(result, 0);   // 86
```

`map` on a missing value short-circuits, so chains like the one above never call the later functions when any step fails.

### 2. `adt.hpp` — pattern matching over variants

Two small tools for working with `std::variant` and generally with callable objects.

| Type / Function | Description |
|---|---|
| `overload<Fs...>` | A struct inheriting from all `Fs` and exposing their `operator()` (a C++17 trick, packaged here). CTAD is provided. |
| `match(variant, fs...)` | `std::visit` wrapper — dispatch a variant to a set of lambdas, selecting by alternative type. |

```cpp
std::variant<int, std::string> v = 42;

auto label = match(v,
    [](int x)      { return "integer: " + std::to_string(x); },
    [](std::string s) { return "string: " + s; });
```

`match` is especially useful with the ADTs in this library:

```cpp
Result<int> r = read_config("port");
int port = match(r.v,
                 [](const std::string& e) { return 8080; },  // default on error
                 [](int p) { return p; });
```

### 3. `either.hpp` — sum types

`Either<E, T>` is a tagged union holding **either** an error of type `E` (index 0) **or** a value of type `T` (index 1).

| Member | Description |
|---|---|
| `static Either ok(T)` / `static Either err(E)` | Constructors (prefer the free `fp::ok`/`fp::err` in `result.hpp`). |
| `bool is_ok() const` | `true` when holding a value. |
| `T const& value() const` / `E const& error() const` | Accessors (UB / throws if the wrong alternative is held — check with `is_ok` first). |
| `map(Either<E,T>, F) -> Either<E, R>` | Transform the value, leaving the error untouched. |

```cpp
Either<std::string, int> success = Either<std::string, int>::ok(21);
auto doubled = map(success, [](int x) { return x * 2; });   // ok(42)

Either<std::string, int> failure = Either<std::string, int>::err("boom");
auto also_fail = map(failure, [](int x) { return x * 2; }); // err("boom")
```

`std::variant<E, T>` is the underlying storage, exposed as the public member `.v`, so you can also destructure with `fp::match` if you prefer.

### 4. `result.hpp` — string-typed errors

A specialization of `Either` where the error is `std::string`, plus free functions and a sequencing helper.

| Type / Function | Description |
|---|---|
| `template<class T> using Result<T> = Either<std::string, T>` | The conventional "function that may fail with a message". |
| `ok(T) -> Result<T>` | Wrap a success value. |
| `err(std::string) -> Result<T>` | Wrap an error message (template arg deduced from context). |
| `sequence(vector<Result<T>>) -> Result<vector<T>>` | Turn a list of results into a result of a list; fails fast on the first error. |

```cpp
Result<int> parse_int(const std::string& s) {
    try { return ok(std::stoi(s)); }
    catch (...) { return err<int>("not a number"); }
}

// chain manually through Either::map
auto doubled = map(parse_int("21"), [](int x) { return x * 2; });   // ok(42)

// collect many results
std::vector<Result<int>> ops = { ok(1), parse_int("2"), parse_int("x") };
auto all = sequence(ops);    // err("not a number") — first failure wins
```

This is the type used by the async helpers in `concurrent.hpp` (see §13).

### 5. `validation.hpp` — accumulating validation

Like `Result`, but the error side is a **list** of messages, and combinators **accumulate** all failures instead of failing fast.

| Type / Function | Description |
|---|---|
| `template<class T> using Validation<T> = Either<std::vector<std::string>, T>` | Error side carries multiple messages. |
| `valid(T) -> Validation<T>` | A passing value. |
| `invalid(std::string) / invalid(vector<string>) -> Validation<T>` | One or many error messages. |
| `validate_all(vector<Validation<T>>) -> Validation<vector<T>>` | Collects all valid values and *all* error messages. |
| `combine2(Validation<A>, Validation<B>, F) -> Validation<result>` | Combine two validations, accumulate both error lists, then call `F(a, b)` if both are valid. |

```cpp
Validation<std::string> name = valid<std::string>("Ada");   // note: explicit T — "Ada" would deduce const char*
auto age   = parse_age("abc");                              // invalid<int>("age is not a number")
auto email = parse_email("x");                      // invalid<std::string>("missing @")

// all messages are gathered, not just the first
auto payload = combine2(combine2(name, age, [](std::string n, int a) {
                                            return Person{n, a, ""};
                                        }),
                        email, [](Person p, std::string e) { p.email = e; return p; });
// -> invalid({"age is not a number", "missing @"})
```

`invalid`'s template parameter is deduced from context; when that is ambiguous, spell it out: `invalid<int>("msg")`.

### 6. `vec.hpp` — vector combinators

Pure, allocation-friendly helpers over `std::vector`.

| Function | Signature | Description |
|---|---|---|
| `map` | `(vector<T>, F) -> vector<R>` | Transform every element. |
| `filter` | `(vector<T>, F) -> vector<T>` | Keep elements passing a predicate. |
| `zip` | `(vector<A>, vector<B>) -> vector<pair<A,B>>` | Pair elements up to the shorter length. |
| `zip_with` | `(vector<A>, vector<B>, F) -> vector<R>` | Combine elements pairwise (length = min). |
| `head` | `(vector<T>) -> optional<T>` | First element, or `nullopt` for an empty vector. |
| `partition` | `(vector<T>, F) -> pair<vector<T>, vector<T>>` | Split into passing / failing elements. |
| `group_by` | `(vector<T>, F) -> unordered_map<K, vector<T>>` | Bucket elements by a key function. |
| `chunk` | `(vector<T>, size_t) -> vector<vector<T>>` | Split into fixed-size chunks (empty result if `size == 0`). |

```cpp
std::vector<int> nums{1, 2, 3, 4, 5, 6};

auto squares = map(nums, [](int x) { return x * x; });             // 1,4,9,16,25,36
auto even    = filter(nums, [](int x) { return x % 2 == 0; });     // 2,4,6
auto pairs   = zip(nums, std::vector<int>{9, 8, 7});               // (1,9),(2,8),(3,7)
auto sums    = zip_with(nums, std::vector<int>{1, 1, 1},
                        [](int a, int b) { return a + b; });       // 2,3,4
auto first   = head(nums);                                         // optional{1}
auto [yes, no] = partition(nums, [](int x) { return x > 3; });     // {4,5,6} / {1,2,3}
auto by_parity = group_by(nums, [](int x) { return x % 2; });      // {0:{2,4,6}, 1:{1,3,5}}
auto pieces  = chunk(nums, 2);                                     // {1,2},{3,4},{5,6}
```

### 7. `ranges.h` — range combinators

Works on any `std::ranges::range` (vector, span, filter view, …). Note the `.h` extension and that it is **not** included by `all.hpp` — include `<fp/ranges.h>` explicitly.

| Function | Signature | Description |
|---|---|---|
| `filter_map` | `(R, F) -> vector<T>` | Map each element with `f` returning `optional<T>`; keep the unwrapped values. |
| `fold_left` | `(R, T init, F) -> T` | Left fold / accumulate. |

```cpp
std::vector<std::string> words{"  hi ", "", " 42 ", "no"};

auto parsed = filter_map(words, [](const std::string& w) -> std::optional<int> {
    auto t = str::trim(w);
    if (t.empty()) return std::nullopt;
    return std::optional<int>{std::stoi(t)};   // throws on "no" — wrap as needed
});
// {42}

auto total = fold_left(parsed, 0, [](int acc, int x) { return acc + x; });  // 42
```

### 8. `string.hpp` — string utilities

Free functions in namespace `fp::str`. All are pure — they take the string by value and return a new one.

| Function | Description |
|---|---|
| `to_lower(s)` / `to_upper(s)` | Case conversion (locale-independent, byte-based). |
| `trim(s, c=' ')` | Remove leading and trailing `c`. |
| `trim_leading(s, c=' ')` / `trim_trailing(s, c=' ')` | Remove only one side. |
| `split(s, delim)` | Split on a single character into `vector<string>`. |
| `join(parts, sep)` | Concatenate a `vector<string>` with a separator. |

```cpp
using fp::str;

to_lower("HELLO");                     // "hello"
trim("  \t hi  ");                     // note: trim removes only ' ' (and the side variants)
split("a,b,c", ',');                   // {"a","b","c"}
join({"2026", "09", "04"}, "-");       // "2026-09-04"
```

> `trim`/`trim_leading` strip the space character only — use `trim_leading(s, '\t')` etc. for other whitespace.

### 9. `combinators.hpp` — identity, const, flip

Tiny higher-order helpers useful when composing callbacks.

| Function | Description |
|---|---|
| `identity(x)` | Returns its argument unchanged. |
| `const_(x)` | Returns a function that ignores its arguments and always yields `x`. |
| `flip(f)` | Returns a function with the first two arguments swapped. |

```cpp
identity(42);                       // 42
auto always_five = const_(5);
always_five(1, 2, 3);               // 5
flip([](int a, int b) { return a - b; })(1, 2);   // 1 (2 - 1)
```

### 10. `compose.hpp` — function composition and pipes

The heart of the library. Build pipelines of one-argument functions three ways.

| Function | Description |
|---|---|
| `compose(f, g)` | Right-to-left composition: `(f ∘ g)(x) == f(g(x))`. |
| `pipe(f, g, h, ...)` | Left-to-right composition: `pipe(f,g,h)(x) == h(g(f(x)))`. |
| `into(x)` | Lift a value into a `Piped<T>`. |
| `out(piped)` | Unwrap the final value. |
| `operator|(Piped<T>, F)` | Apply a function inside a pipeline; if `F` returns `void`, the value is passed through. |
| `tap(f)` | Run a side effect on `x`, returning `x` unchanged. |

```cpp
using namespace fp;

// classic composition
auto add1 = [](int x) { return x + 1; };
auto dbl  = [](int x) { return x * 2; };
compose(add1, dbl)(3);        // 7  = add1(dbl(3))
pipe(dbl, add1)(3);           // 7  = add1(dbl(3))

// value piping — the idiomatic form
int answer = out(into(3)
    | [](int x) { return x * 3; }      // 9
    | [](int x) { return x + 1; }      // 10
    | [](int x) { return x * 4; }      // 40
    | [](int x) { return x + 2; });    // 42

// tap for debugging / side effects without breaking the chain
int n = out(into(5)
    | tap([](int x) { std::cout << "got " << x << "\n"; })
    | [](int x) { return x * 2; });    // 10
```

`const_`, `identity`, `curry` (next section) and `memoize` all compose naturally with `pipe`/`|`.

### 11. `curry.hpp` — partial application

`curry(f, bound...)` returns a function that accumulates arguments until `f` is invocable, then calls it.

| Function | Description |
|---|---|
| `curry(F)` | Returns a curried version of `F`. |

```cpp
auto add3 = curry([](int a, int b, int c) { return a + b + c; });

add3(1, 2, 3);          // 6 — all args at once
add3(1)(2)(3);          // 6 — one at a time
auto add1 = add3(1);    // partially applied
add1(2, 3);             // 6
```

The decision to call vs. keep currying is made by `std::is_invocable`, so default-argument and overload-sensitive functions behave as expected.

### 12. `memoize.hpp` — cached functions

`memoize<Arg>(f)` caches results in a shared `unordered_map<Arg, Ret>`; calling with a previously seen argument returns the cached value.

| Function | Description |
|---|---|
| `memoize<Arg>(F) -> memoized F` | Wraps `F`, which must take one argument of type `Arg` (hashable). |

```cpp
// recursive memoization needs a std::function so the lambda can call itself
std::function<long long(int)> fib = memoize<int>([&fib](int n) -> long long {
    return n < 2 ? n : fib(n - 1) + fib(n - 2);
});
long long f20 = fib(20);   // 6765, each sub-result computed once
```

Useful when `f` is expensive (parsing, lookups) and inputs repeat. Not thread-safe — guard with an external mutex if shared across threads.

### 13. `concurrent.hpp` — parallel map, futures, actors

Requires linking with `-pthread`.

| Type / Function | Description |
|---|---|
| `par_map(vector<T>, F, threads = hardware_concurrency()) -> vector<R>` | Map with a thread pool of `std::async` tasks; chunks the input and preserves order. Falls back to a sequential loop for empty inputs or `threads <= 1`. |
| `template<class T> using AsyncResult<T> = std::future<Result<T>>` | A future holding a `Result`. |
| `async_map(AsyncResult<T>, F) -> AsyncResult<R>` | Monadic map over an async `Result`: propagates the error without running `F`, runs `F` on success. |
| `actor<Msg, State>` | An actor with a single worker thread. Constructor: `actor(State initial, Handler)` where `Handler = std::function<State(State, Msg)>`. |

```cpp
// parallel map
std::vector<int> ids{1, 2, 3, 4, 5, 6, 7, 8};
auto rendered = par_map(ids, [](int id) { return render(id); }, 4);

// chained async results
AsyncResult<int> future = std::async(std::launch::async, [] {
    return ok<int>(calculate());
});

auto next = async_map(std::move(future), [](int x) { return x * 2; });
auto r = next.get();
if (r.is_ok()) { /* r.value() */ }

// actor: state machine with sequential message handling
using Msg = int;                       // add this many
actor<Msg, int> counter(0, [](int state, Msg m) { return state + m; });
counter.Send(5);
counter.Send(10);
std::this_thread::sleep_for(std::chrono::milliseconds(10));
counter.snapshot();                    // 15
```

The `actor` communicates via a mutex-protected queue and condition variable; `Send` never blocks the caller, and `snapshot` can be polled from any thread. The destructor stops the worker loop and joins the thread, so destruction waits for in-flight messages to be processed.

### 14. `simd.hpp` — SIMD-accelerated mapping

Vectorized in-place mapping using C++ `<experimental/simd>` (libstdc++ `native_simd`).

| Type / Function | Description |
|---|---|
| `template<class T> using vec<T> = std::experimental::native_simd<T>` | The native SIMD vector for `T`. |
| `map_inplace(vector<T>&, F)` | Apply `f` to elements in SIMD-width chunks; `f` takes a `vec<T>` and returns one, so it already vectorizes; the tail (fewer than one full SIMD chunk) is processed element-wise. |

```cpp
std::vector<double> samples{0.1, 0.2, 0.3, 0.4, 0.5};

map_inplace(samples, [](fp::vec<double> v) {
    return v * 2.0 + 1.0;      // expressed in SIMD lanes
});
// {1.2, 1.4, 1.6, 1.8, 2.0}
```

Notes:

- `element_aligned` requires only element alignment (which `std::vector` naturally provides); SIMD-width chunks are read/written in full lanes, so sizes that are a multiple of the SIMD width avoid the scalar tail.
- Requires GCC with libstdc++ `experimental/simd` support (GCC 11+) and is **not** included by `all.hpp` — include `<fp/simd.hpp>` explicitly.
- Only arithmetic types (`float`, `double`, `int`, …) are supported by `native_simd`.

---

## Limitations and notes

- **Header-only, no ABI** — everything is templates or `inline`; nothing is ever linked.
- **Include hygiene**: `all.hpp` does **not** include `ranges.h` or `simd.hpp` (newer modules). Include `<fp/ranges.h>` and `<fp/simd.hpp>` explicitly when you need them.
- **`Either` accessors** (`value()`/`error()`) throw/UB on the wrong alternative; always guard with `is_ok()` or destructure with `match`.
- **`memoize`** is not thread-safe.
- **`actor`** joins its worker thread in the destructor (after processing queued messages); destruction blocks until the handler finishes the current message.
- **Threads**: `concurrent.hpp` needs `-pthread`; `par_map` spawns one `std::async` per chunk — for tiny vectors keep `threads` small or pass `1`.
- The public `include/forgefp/` tree contains an older subset (`adt`, `either`, `maybe`, `result`); prefer `src/fp/` for the current API. Both are compatible (same namespace, same signatures).

---

## License

See repository metadata. ForgeFP is built with [Forge](https://forge) (see `forge.lua`).
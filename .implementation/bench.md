# bench.md — benchmarking the performance-specific functions

SIMD and concurrency code has one job: be fast. Until there are numbers,
the README's "SIMD-accelerated" and "parallel map" claims are marketing.
This file defines how to measure them **without adding any dependency**
(consistent with the library's header-only, zero-dep contract).

## Status

- Infra **in place**: `bench/bench.hpp`, `bench/simd_bench.cpp`,
  `bench/concurrent_bench.cpp`, `scripts/run_bench.sh`, and the
  `["bench"]`/`["bench-pinned"]` forge scripts.
- `simd_bench.cpp` compiles the `map_inplace` baseline now; the
  `reduce`/`dot` cases are commented until `simd.md` #1/#2 land.
- `concurrent_bench.cpp` covers sequential/`par_map`/actor now; the
  `ThreadPool` cases are commented until `concurrent.md` #1–#3 land.
- `forge run bench` / `bash scripts/run_bench.sh [cpu]` to run.

## What to benchmark (per module)

| Module | Function | Baseline to beat | Why it matters |
|---|---|---|---|
| `simd` | `reduce` | `std::accumulate` | The whole point of the module. |
| `simd` | `dot` | naive product-sum loop | Fusable multiply-accumulate. |
| `simd` | `map_inplace` / `map_to` | element-wise scalar loop | Vector width vs. scalar tail. |
| `simd` | `map_inplace_fixed` (masked tail) | `map_inplace` (scalar tail) | Prove the masked tail is worth it. |
| `concurrent` | `par_map` (ThreadPool) | sequential `fp::map` | Must scale with `-j`, not add overhead. |
| `concurrent` | `par_reduce` | `fold_left` | Associativity + parallelism win. |
| `concurrent` | `par_for_each` | plain loop | Side-effect parallelism. |
| `concurrent` | `actor` `Send` drain | — | Throughput in msgs/sec (queue + cv cost). |
| `concurrent` | `Channel` send/recv pairs | — | Producer/consumer overhead. |
| `vec` | `group_by` / `chunk` / `partition` (1M elements) | `unordered_map` inserts / raw loops | Allocation-bound cases worth profiling. |
| `string` | `split` / `join` on 1MB text | naive impl | Byte-path hotness. |

## The harness — `bench/bench.hpp` (zero dependencies)

```cpp
#pragma once
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>

namespace bench {

inline void keep(void const volatile* ptr) {
  asm volatile("" : : "r"(ptr) : "memory");
}

template <class F>
double time_rep(F&& f, std::size_t reps) {
  auto t0 = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < reps; ++i)
    f();
  auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps;
}

// Calibrates reps so one run's TOTAL time >= target_ns, then reports the
// BEST of `samples` runs (least-noise estimator). Default target: 100 ms.
// Calibration compares TOTAL (t * reps), not per-op time — per-op time is
// constant once an op is warmed up, so comparing it to the target doubles
// reps forever. The cap is a second safety net against eliminated cases.
template <class F>
double measure(char const* name, std::size_t target_ns, int samples, F f) {
  f();  // warmup + correctness smoke
  double best = 0.0;
  for (int s = 0; s < samples; ++s) {
    std::size_t reps = 1;
    double t = 0.0;
    while ((t = time_rep(f, reps)) * reps < target_ns && reps < (1ull << 32))
      reps *= 2;
    best = (best == 0.0) ? t : std::min(best, t);
  }
  std::printf("%-44s %10.2f ns/op\n", name, best);
  std::fflush(stdout);  // keep logs live when piped / killed
  return best;
}

template <class F>
double measure(char const* name, F f) {
  return measure(name, 100'000'000, 5, std::move(f));
}

}  // namespace bench
```

Design notes (each choice is deliberate, not cargo-culting):

- **`keep()`** — the compiler may forward-solve a pure fold and delete the
  whole loop. Writing the result through `keep(&result)` (opaque asm) forces
  the computation to actually run. Without it the numbers are fake.
- **Calibration, not fixed reps** — ops differ by orders of magnitude
  (2 ns vs 20 ms). Doubling reps until a run's *total* time outweighs timer
  noise keeps every measurement meaningful without hand-tuning. (Calibrate
  on total; the reported number is per-op.)
- **Best-of-5, not average** — averages include scheduler jitter and clock
  interrupts; the minimum is the run closest to the true cost. (Report best
  for "how fast can it go", which is what SIMD claims are about.)
- **`steady_clock`, not `system_clock`** — monotonic, immune to wall-clock
  adjustments.

## Worked file 1 — `bench/simd_bench.cpp`

```cpp
#include <fp/simd.hpp>
#include <numeric>
#include <vector>

#include "bench.hpp"

int main() {
  // sizes: one multiple of every lane width (1<<22), one with a tail (+3)
  std::vector<double> a(1 << 22);
  std::vector<double> b(1 << 22);
  for (std::size_t i = 0; i < a.size(); ++i) {
    a[i] = 0.5 + static_cast<double>(i % 13);
    b[i] = 1.0 - static_cast<double>(i % 7);
  }

  std::printf("reduce  (fp64, %zu elems)\n", a.size());
  bench::measure("std::accumulate", [&] {
    double r = std::accumulate(a.begin(), a.end(), 0.0);
    bench::keep(&r);
  });
  bench::measure("fp::reduce", [&] {
    double r = fp::reduce(a);
    bench::keep(&r);
  });

  std::printf("dot     (fp64, %zu elems)\n", a.size());
  bench::measure("naive product-sum", [&] {
    double r = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
      r += a[i] * b[i];
    bench::keep(&r);
  });
  bench::measure("fp::dot", [&] {
    double r = fp::dot(a, b);
    bench::keep(&r);
  });

  std::printf("map_inplace (fp64, %zu elems, tail included)\n", a.size() + 3);
  auto v = a;
  v.push_back(0.1);
  v.push_back(0.2);
  v.push_back(0.3);
  bench::measure("scalar loop (*2+1)", [&] {
    for (auto& x : v)
      x = x * 2.0 + 1.0;
    bench::keep(&v[0]);
  });
  bench::measure("fp::map_inplace (*2+1)", [&] {
    fp::map_inplace(v, [](fp::vec<double> x) { return x * 2.0 + 1.0; });
    bench::keep(&v[0]);
  });
}
```

## Worked file 2 — `bench/concurrent_bench.cpp`

```cpp
#include <fp/concurrent.hpp>
#include <fp/vec.hpp>
#include <thread>

#include "bench.hpp"

int main() {
  std::vector<int> ids;
  ids.reserve(1 << 20);
  for (int i = 0; i < (1 << 20); ++i)
    ids.push_back(i);

  auto work = [](int x) { return static_cast<int>((x * 2654435761u) % 97); };

  bench::measure("std::for_each (sequential)", [&] {
    long long acc = 0;
    for (auto x : ids)
      acc += work(x);
    bench::keep(&acc);  // consume the result — a discarded loop is deleted
  });
  bench::measure("fp::map (sequential)", [&] {
    auto r = fp::map(ids, work);
    bench::keep(&r[0]);
  });
  bench::measure("fp::par_map (std::async, 8)", [&] {
    auto r = fp::par_map(ids, work, 8);
    bench::keep(&r[0]);
  });

  // Once ThreadPool lands (concurrent.md #1):
  // fp::ThreadPool pool(8);
  // bench::measure("fp::par_map (ThreadPool, 8)", [&] {
  //   auto r = fp::par_map(pool, ids, work);
  //   bench::keep(&r[0]);
  // });

  std::printf("actor drain (100k messages)\n");
  constexpr int kMsgs = 100'000;
  fp::actor<int, long long> counter(0, [](long long s, int m) { return s + m; });
  bench::measure("counter.Send -> drained", [&] {
    for (int i = 0; i < kMsgs; ++i)
      counter.Send(1);
    while (counter.snapshot() < kMsgs)
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    bench::keep(&counter);
  });
}
```

Note the `bench::keep(&ids[0])` trick in the dead-code-prone sequential loop:
`work(x)` has no observable side effect, so without the keep the loop is
optimized away entirely — the classic parallel-benchmark bug.

## Building and running — `scripts/run_bench.sh` (forge-wired)

```bash
#!/usr/bin/env bash
set -euo pipefail
mkdir -p build
CPU="${1:-}"   # optional: cores to pin, e.g. "2" or "0-3"
PIN=()
[ -n "$CPU" ] && PIN=(taskset -c "$CPU")

g++ -std=c++20 -O2 -march=native -Isrc -Ibench \
    bench/simd_bench.cpp -o build/simd_bench
g++ -std=c++20 -O2 -march=native -Isrc -Ibench \
    bench/concurrent_bench.cpp -pthread -o build/concurrent_bench

echo "== simd =="
"${PIN[@]}" ./build/simd_bench
echo "== concurrent =="
"${PIN[@]}" ./build/concurrent_bench
```

Then one `forge run` entry:

```lua
-- forge.lua
scripts = {
  ["bench"] = "bash scripts/run_bench.sh",
  -- pinned, quieter numbers:
  ["bench-pinned"] = "bash scripts/run_bench.sh 2",
},
```

```bash
forge run bench
```

### Rules for trustworthy numbers

1. **Build with `-O2 -march=native`** — `native_simd` is a no-op without a
   target CPU; numbers without `-march=native` are the scalar loop wearing a
   costume.
2. **Pin the process** (`taskset -c 2`) when comparing close numbers; note
   that `par_map` in a pin of one core shows *worse* than sequential — that's
   correct and expected, don't "fix" it.
3. **Record the machine** — every benchmark banner should print CPU + flags
   (`__VERSION__`, `std::experimental::native_simd::size()`), because
   `march=native` numbers never transfer between machines. Keep the raw
   output in the commit message or a `bench/README.md` table.
4. **Never assert on ratios in CI** — benchmarks are informational on CI
   (shared runners are too noisy); gate on "compiles + runs + sane magnitude",
   not on relative speed.
5. **Compare apples to apples** — identical input data, identical flags,
   same code path except the one variable under test.

## Alternative: Google Benchmark (only if you want the mature toolchain)

Forge can pull it in as a normal FetchContent dependency (same mechanism as
googletest):

```lua
-- forge.lua — optional, only if you accept the dependency
dependencies = {
  direct = {
    benchmark = {
      git = "https://github.com/google/benchmark.git",
      tag = "v1.9.1",
      target = "benchmark::benchmark",
    },
  },
},
```

Plus a custom target via `forge.add_cmake` (never edit generated CMake):

```lua
forge.add_cmake([[
  add_executable(forgefp_bench bench/simd_bench.cpp)
  target_include_directories(forgefp_bench PRIVATE src include)
  target_compile_options(forgefp_bench PRIVATE -O2 -march=native)
  target_link_libraries(forgefp_bench PRIVATE forgefp benchmark::benchmark)
]])
```

Google Benchmark gives you `BENCHMARK(...)` macros, automatic arg ranges,
and statistical reporting — in exchange for a dependency and its compile
time. The zero-dep harness above covers the same ground in ~40 lines; start
there, upgrade only if the reporting matters.

## Definition of done for benchmark work

- [ ] `bench/bench.hpp` + at least `bench/simd_bench.cpp` and
      `bench/concurrent_bench.cpp` (update once `ThreadPool`/`reduce`/`dot`
      land in `src/`)
- [ ] `scripts/run_bench.sh` + `["bench"]` forge script entry
- [ ] Banner prints CPU/compiler/SIMD-width so results are interpretable
- [ ] CI job runs the benches (informational; see testing.md §6)
- [ ] README records the headline numbers (e.g. `map_inplace` ≥ 4× scalar)
      with the machine it was measured on
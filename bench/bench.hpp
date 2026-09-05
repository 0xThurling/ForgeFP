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
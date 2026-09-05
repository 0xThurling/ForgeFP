// Benchmark the concurrency module against sequential baselines.
//
// ThreadPool cases are commented out until it lands
// (see .implementation/concurrent.md #1-#3) — uncomment them afterwards.
#include <fp/concurrent.hpp>
#include <fp/vec.hpp>
#include <thread>

#include "bench.hpp"

int main() {
  std::printf("hardware concurrency: %u\n",
              std::thread::hardware_concurrency());

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
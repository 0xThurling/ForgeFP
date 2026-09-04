#pragma once
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "result.hpp"

namespace fp {
template <class T, class F>
auto par_map(std::vector<T> const &v, F f,
             std::size_t threads = std::thread::hardware_concurrency()) {
  using R = std::invoke_result_t<F, T>;
  if (v.empty() || threads <= 1) {
    std::vector<R> out;
    out.reserve(v.size());
    for (auto const &x : v)
      out.push_back(f(x));
    return out;
  }

  std::size_t n = v.size();
  std::size_t chunk = (n + threads - 1) / threads;
  std::vector<std::future<std::vector<R>>> futures;

  futures.reserve((n + chunk - 1) / chunk);

  for (std::size_t start = 0; start < n; start += chunk) {
    std::size_t end = std::min(n, start + chunk);
    futures.emplace_back(std::async(std::launch::async, [&, start, end]() {
      std::vector<R> out;
      out.reserve(end - start);
      for (std::size_t i = start; i < end; ++i)
        out.push_back(f(v[i]));
      return out;
    }));
  }

  std::vector<R> result;
  result.reserve(n);
  for (auto &fut : futures) {
    auto part = fut.get();
    result.insert(result.end(), part.begin(), part.end());
  }
  return result;
}

template <class Msg, class State> class actor {
public:
  using Handler = std::function<State(State, Msg)>;

  actor(State initial, Handler h)
      : state_(std::move(initial)), handler_(std::move(h)), running_(true),
        thr_([this] { this->loop(); }) {}

  ~actor() {
    std::lock_guard lock(mu_);
    running_ = false;
    cv_.notify_all();
  }

  void Send(Msg m) {
    std::lock_guard lock(mu_);
    queue_.push(std::move(m));
    cv_.notify_one();
  }

  State snapshot() const {
    std::lock_guard lock(mu_);
    return state_;
  }

private:
  void loop() {
    std::unique_lock lock(mu_);
    while (running_) {
      cv_.wait(lock, [&] { return !queue_.empty() || !running_; });
      while (!queue_.empty()) {
        Msg m = std::move(queue_.front());
        queue_.pop();
        State new_state = handler_(state_, m);
        state_ = std::move(new_state);
      }
    }
  }

  mutable std::mutex mu_;
  std::condition_variable cv_;
  std::queue<Msg> queue_;
  State state_;
  Handler handler_;
  bool running_;
  std::thread thr_;
};

template <class T> using AsyncResult = std::future<Result<T>>;

template <class T, class F>
AsyncResult<std::invoke_result_t<F, T>> async_map(AsyncResult<T> fut, F f) {
  using R = std::invoke_result_t<F, T>;
  return std::async(std::launch::async,
                    [f = std::move(f), fut = std::move(fut)]() mutable {
                      auto r = fut.get();

                      if (!r.is_ok())
                        return err<R>(r.error());

                      return ok<R>(f(r.value()));
                    });
}
} // namespace fp

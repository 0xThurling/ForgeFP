#pragma once
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace fp {

template <std::ranges::range R, class F> auto filter_map(R &&r, F f) {
  using T = std::invoke_result_t<F, std::ranges::range_value_t<R>>;
  std::vector<typename T::value_type> out;
  for (auto &&x : r) {
    if (auto v = f(x))
      out.push_back(*v);
  }

  return out;
}

template <std::ranges::range R, class T, class F>
T fold_left(R &&r, T init, F f) {
  for (auto &&x : r)
    init = f(std::move(init), x);
  return init;
}

template <std::ranges::range R, class F> auto map(R &&r, F f) {
  using T = std::ranges::range_value_t<R>;
  std::vector<std::invoke_result_t<F, T>> out;
  if constexpr (std::ranges::sized_range<R>)
    out.reserve(std::ranges::size(r));
  for (auto &&x : r)
    out.push_back(f(std::forward<decltype(x)>(x)));
  return out;
}

template <std::ranges::range R, class F> auto filter(R &&r, F pred) {
  using T = std::ranges::range_value_t<R>;
  std::vector<T> out;
  if constexpr (std::ranges::sized_range<R>) {
    out.reserve(std::ranges::size(r));
  }
  for (auto &&x : r)
    if (pred(x))
      out.push_back(std::forward<decltype(x)>(x));
  return out;
}

template <std::ranges::range R> auto take(R &&r, size_t n) {
  using T = std::ranges::range_value_t<R>;
  std::vector<T> out;
  size_t i = 0;
  for (auto it = std::ranges::begin(r); it != std::ranges::end(r) && i < n;
       ++it, ++i)
    out.push_back(*it);
  return out;
}

template <std::ranges::range R> auto drop(R &&r, size_t n) {
  using T = std::ranges::range_value_t<R>;
  std::vector<T> out;
  auto it = std::ranges::begin(r);
  auto end = std::ranges::end(r);
  for (size_t i = 0; i < n && it != end; ++i, ++it) {
  }
  for (; it != end; ++it)
    out.push_back(*it);
  return out;
}

template <std::ranges::range R> auto concat(R &&rs) {
  using Inner = std::ranges::range_value_t<R>;
  static_assert(std::ranges::range<Inner>);
  using T = std::ranges::range_value_t<Inner>;
  std::vector<T> out;
  for (auto &&xs : rs)
    for (auto &&x : xs)
      out.push_back(std::forward<decltype(x)>(x));
  return out;
}
} // namespace fp

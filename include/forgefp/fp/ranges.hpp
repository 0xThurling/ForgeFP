#pragma once
#include <ranges>
#include <type_traits>
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
} // namespace fp

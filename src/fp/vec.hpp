#pragma once
#include <algorithm>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace fp {
template <class T, class F> auto map(std::vector<T> const &v, F f) {
  std::vector<std::invoke_result_t<F, T>> out;
  out.reserve(v.size());
  for (auto const &x : v)
    out.push_back(f(x));
  return out;
}

template <class T, class F>
std::vector<T> filter(std::vector<T> const &v, F pred) {
  std::vector<T> out;
  for (auto const &x : v)
    if (pred(x))
      out.push_back(x);
  return out;
}

template <class A, class B>
std::vector<std::pair<A, B>> zip(std::vector<A> const &a,
                                 std::vector<B> const &b) {
  std::vector<std::pair<A, B>> out;
  auto n = std::min(a.size(), b.size());
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
    out.emplace_back(a[i], b[i]);
  return out;
}

template <class T> std::optional<T> head(std::vector<T> const &v) {
  if (v.empty())
    return std::nullopt;
  return v.front();
}

template <class T, class F> auto partition(std::vector<T> const &v, F f) {
  std::vector<T> yes, no;
  for (auto const &x : v)
    (pred(x) ? yes : no).push_back(x);
  return std::pair{yes, no};
}

template <class T, class F> auto group_by(std::vector<T> const &v, F key_fn) {
  using K = std::invoke_result_t<F, T>;
  std::unordered_map<K, std::vector<T>> out;
  for (auto const &x : v)
    out[key_fn(x)].push_back(x);
  return out;
}

template <class T>
std::vector<std::vector<T>> chunk(std::vector<T> const &v, size_t size) {
  std::vector<std::vector<T>> out;
  if (size == 0)
    return out;
  for (size_t i = 0; i < v.size(); i += size) {
    out.emplace_back(v.begin() + i, v.begin() + std::min(v.size(), i + size));
  }
  return out;
}

template <class A, class B, class F>
auto zip_with(std::vector<A> const &a, std::vector<B> const &b, F f) {
    std::vector<std::invoke_result_t<F, A, B>> out;
    auto n = std::min(a.size(), b.size());
    out.resize(n);
    for (size_t i = 0; i < n; ++i) out.push_back(f(a[i], b[i]));
    return out;
}
} // namespace fp

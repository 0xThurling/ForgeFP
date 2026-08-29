#pragma once
#include <optional>
#include <type_traits>

namespace fp {
template<class T, class F>
auto map(std::optional<T> const &o, F f) -> std::optional<std::invoke_result_t<F, T>> {
  if (!o)
    return std::nullopt;
  return f(*o);
}

template<class T, class F>
auto and_then(std::optional<T> const &o, F f) -> std::invoke_result_t<F, T> {
  if (!o)
    return std::nullopt;
  return f(*o);
}

template<class T>
T or_else(std::optional<T> const& o, T fallback) {
    return o.value_or(fallback);
}

template<class T, class F>
std::optional<T> filter(std::optional<T> const& o, F pred) {
    if (o && pred(*o)) return o;
    return std::nullopt;
}
} // namespace fp

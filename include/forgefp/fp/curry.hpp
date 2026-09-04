#pragma once
#include <type_traits>
#include <utility>

namespace fp {
template <class F, class... Bound> auto curry(F f, Bound... bound) {
  return [f, bound...](auto &&...rest) {
    if constexpr (std::is_invocable_v<F, Bound..., decltype(rest)...>) {
      return f(bound..., std::forward<decltype(rest)>(rest)...);
    } else {
      return curry(f, bound..., std::forward<decltype(rest)>(rest)...);
    }
  };
}
} // namespace fp

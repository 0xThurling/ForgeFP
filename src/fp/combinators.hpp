#pragma once

#include <utility>
namespace fp {
template <class T> T identity(T x) { return x; }

template <class T> auto const_(T x) {
  return [x](auto &&...) { return x; };
}

template <class F> auto flip(F f) {
  return [f](auto a, auto b) { return f(b, a); };
}

template <class F, class G> auto on(F f, G g) {
  return [f = std::move(f), g = std::move(g)](auto const &x,
                                              auto const &y) -> decltype(auto) {
    return f(g(x), g(y));
  };
}
} // namespace fp

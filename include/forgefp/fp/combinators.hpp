#pragma once

namespace fp {
template <class T> T identity(T x) { return x; }

template <class T> auto const_(T x) {
  return [x](auto &&...) { return x; };
}

template <class F> auto flip(F f) {
  return [f](auto a, auto b) { return f(b, a); };
}
} // namespace fp

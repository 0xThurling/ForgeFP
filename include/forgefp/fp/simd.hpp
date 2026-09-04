#pragma once
#include <cstddef>
#include <experimental/bits/simd.h>
#include <experimental/simd>
#include <vector>

namespace fp {
template <class T> using vec = std::experimental::native_simd<T>;

template <class T, class F> void map_inplace(std::vector<T> &data, F f) {
  using V = vec<T>;
  std::size_t width = V::size();
  std::size_t n = data.size();
  std::size_t i = 0;

  for (; i + width <= n; i += width) {
    V chunk;
    chunk.copy_from(&data[i], std::experimental::element_aligned);
    chunk = f(chunk);
    chunk.copy_to(&data[i], std::experimental::element_aligned);
  }

  // Tail elements
  for (; i < n; ++i) {
    V v_single(data[i]);
    v_single = f(v_single);
    data[i] = v_single[0];
  }
}
} // namespace fp

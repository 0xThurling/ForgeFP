#pragma once
#include <type_traits>
#include <utility>
#include <variant>

namespace fp {
template <class E, class T> struct Either {
  std::variant<E, T> v;

  bool is_ok() const { return v.index() == 1; }
  T const &value() const { return std::get<1>(v); }
  E const &error() const { return std::get<0>(v); }

  static Either ok(T t) {
    return Either{std::variant<E, T>(std::in_place_index<1>, std::move(t))};
  }

  static Either err(E e) {
    return Either{std::variant<E, T>(std::in_place_index<0>, std::move(e))};
  }
};

template <class E, class T, class F>
auto map(Either<E, T> const &e, F f) -> Either<E, std::invoke_result_t<F, T>> {
  using R = std::invoke_result_t<F, T>;

  if (!e.is_ok())
    return Either<E, R>::err(e.error());

  return Either<E, R>::ok(f(e.value()));
}
} // namespace fp

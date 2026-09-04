#pragma once
#include "either.hpp"
#include <string>
#include <type_traits>
#include <vector>

namespace fp {
template <class T> using Validation = Either<std::vector<std::string>, T>;

template <class T> Validation<T> valid(T t) {
  return Validation<T>::ok(std::move(t));
}

template <class T> Validation<T> invalid(std::string msg) {
  return Validation<T>::err(std::move(msg));
}

template <class T> Validation<T> invalid(std::vector<std::string> msgs) {
  return Validation<T>::err(std::move(msgs));
}

template <class T>
Validation<std::vector<T>> validate_all(std::vector<Validation<T>> const &vs) {
  std::vector<std::string> errs;
  std::vector<T> out;

  for (auto const &v : vs) {
    if (v.is_ok())
      out.push_back(v.value());
    else
      errs.insert(errs.end(), v.error().begin(), v.error().end());
  }

  return errs.empty() ? Validation<std::vector<T>>::ok(std::move(out))
                      : Validation<std::vector<T>>::err(std::move(errs));
}

template <class A, class B, class F>
auto combine2(Validation<A> const &a, Validation<B> const &b, F make)
    -> Validation<std::invoke_result_t<F, A, B>> {
  std::vector<std::string> errs;
  if (!a.is_ok())
    errs.insert(errs.end(), a.error().begin(), a.error().end());
  if (!b.is_ok())
    errs.insert(errs.end(), b.error().begin(), b.error().end());
  if (!errs.empty())
    return Validation<std::invoke_result_t<F, A, B>>::err(errs);
  return Validation<std::invoke_result_t<F, A, B>>::ok(
      make(a.value(), b.value()));
}
} // namespace fp

#pragma once
#include "either.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace fp {
template<class T>
using Result = Either<std::string, T>;

template<class T>
Result<T> ok(T t) {
    return Result<T>::ok(std::move(t));
}

template<class T>
Result<T> err(std::string msg) {
    return Result<T>::err(std::move(msg));
}

template<class T>
Result<std::vector<T>> sequence(std::vector<Result<T>> const& rs) {
    std::vector<T> out;
    out.reserve(rs.size());
    for (auto const& r : rs) {
        if (!r.is_ok()) return err<std::vector<T>>(r.error());
        out.push_back(r.value());
    }
    return ok(std::move(out));
}
}

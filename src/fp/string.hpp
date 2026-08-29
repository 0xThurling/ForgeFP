#pragma once
#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>

namespace fp::str {
inline std::string to_lower(std::string s) {
  std::ranges::transform(s, s.begin(),
                         [](unsigned char c) { return std::tolower(c); });

  return s;
}

inline std::string to_upper(std::string s) {
  std::ranges::transform(s, s.begin(),
                         [](unsigned char c) { return std::toupper(c); });

  return s;
}

inline std::string trim_trailing(std::string s, char c = ' ') {
  while (!s.empty() && s.back() == c)
    s.pop_back();
  return s;
}

inline std::string trim_leading(std::string s, char c = ' ') {
  s.erase(0, s.find_first_not_of(c));
  return s;
}

inline std::string trim(std::string s) {
  return trim_trailing(trim_leading(std::move(s)));
}
} // namespace fp::str

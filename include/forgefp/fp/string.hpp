#pragma once
#include <algorithm>
#include <cctype>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

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

inline std::vector<std::string> split(std::string const &s, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;

  while (std::getline(ss, item, delim))
    out.push_back(item);

  return out;
}

inline std::string join(std::vector<std::string> const &parts,
                        std::string const &sep) {
  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) {
      out += sep;
    }
    out += parts[i];
  }
  return out;
}
} // namespace fp::str

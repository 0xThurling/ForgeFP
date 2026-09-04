#pragma once
#include <algorithm>
#include <cctype>
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

inline std::vector<std::string> split(std::string const &s,
                                      std::string const &delim) {
  std::vector<std::string> out;
  size_t start = 0, pos;
  while ((pos = s.find(delim, start)) != std::string::npos) {
    out.push_back(s.substr(start, pos - start));
    start = pos + delim.size();
  }
  out.push_back(s.substr(start));
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

inline std::string strip_prefix(std::string s, std::string const &prefix) {
  if (s.rfind(prefix, 0) == 0)
    s.erase(0, prefix.size());
  return s;
}

inline std::string strip_suffix(std::string s, std::string const &suffix) {
  if (s.size() >= suffix.size() &&
      s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0)
    s.erase(s.size() - suffix.size());
  return s;
}

inline std::string replace_all(std::string s, std::string const &from,
                               std::string const &to) {
  if (from.empty())
    return s;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
  return s;
}

inline std::vector<std::string> lines(std::string const &s) {
  return split(s, '\n');
}

inline std::string repeat(std::string const &s, size_t n) {
  std::string out;
  out.reserve(s.size() * n);
  for (size_t i = 0; i < n; ++i)
    out += s;
  return out;
}

inline std::string pad_left(std::string s, size_t width, char c = ' ') {
  if (s.size() < width)
    s.insert(0, width - s.size(), c);
  return s;
}

inline std::string pad_right(std::string s, size_t width, char c = ' ') {
  if (s.size() < width)
    s.append(width - s.size(), c);
  return s;
}
} // namespace fp::str

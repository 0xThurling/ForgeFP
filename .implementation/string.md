# string.hpp — implementation status

Current surface (`fp::str`): `to_lower`, `to_upper`, `trim`, `trim_leading`,
`trim_trailing`, `split(char)` + `split(string)`, `join`, `lines`,
`strip_prefix`, `strip_suffix`, `replace_all`, `repeat`, `pad_left`,
`pad_right`.

## Implemented (in `src/fp/string.hpp`)

| Feature | Status |
|---|---|
| `split(string delim)` | done |
| `lines` | done |
| `strip_prefix` / `strip_suffix` | done |
| `replace_all` | done |
| `repeat` | done |
| `pad_left` / `pad_right` | done |

## Backlog

## `to_int` / `to_double` — safe parsing to `Result` [P1]

```
Result<int> to_int(std::string const& s); Result<double> to_double(std::string const& s)
```

**Why:** `std::stoi` throws (or silently stops early on `"42x"`); every
consumer re-writes `try/catch` + whitespace trimming. A `result.hpp`-based
parser is the module's natural complement — it turns the README's own example
("throws on `"no"` — wrap as needed" in `ranges.hpp`) into one call:
`filter_map(words, str::to_int)`.

**Implementation** — needs `#include "result.hpp"`; also solves the
`ranges.hpp` README example

```cpp
inline Result<int> to_int(std::string const& s) {
  std::string t = trim(s);
  if (t.empty())
    return err<int>("not a number");
  try {
    return ok<int>(std::stoi(t));
  } catch (...) {
    return err<int>("not a number");
  }
}

inline Result<double> to_double(std::string const& s) {
  std::string t = trim(s);
  if (t.empty())
    return err<double>("not a number");
  try {
    return ok<double>(std::stod(t));
  } catch (...) {
    return err<double>("not a number");
  }
}
```

## `reverse` [P2]

```
std::string reverse(std::string s)
```

**Why:** Palindrome checks, right-to-left parsing, display fixes; one
`std::reverse` on a copy. The vector module has `reverse`; strings should
match for uniformity.

**Implementation**

```cpp
inline std::string reverse(std::string s) {
  std::reverse(s.begin(), s.end());
  return s;
}
```

## `truncate` / ellipsis [P2]

```
std::string truncate(std::string s, size_t max, std::string tail = "...")
```

**Why:** Log previews, UI labels, and error messages that must not be huge.
Needs a documented contract for max ≤ tail length, and byte-vs-char semantics
(byte-based, like the rest of the module's `tolower` family).

**Implementation** — total for every `max`; byte-based (matches the module's convention)

```cpp
inline std::string truncate(std::string s, size_t max, std::string tail = "...") {
  if (s.size() <= max)
    return s;
  if (max <= tail.size())
    return tail.substr(0, max);
  return s.substr(0, max - tail.size()) + tail;
}
```

## `capitalize` / `title` [P2]

```
std::string capitalize(std::string s)                 // "hello" -> "Hello"
std::string title(std::string s)                      // "hello world" -> "Hello World"
```

**Why:** Natural partner of `to_lower`/`to_upper`; the module is currently
all-or-nothing on case, with no middle ground. Byte-based per file's
established convention.

**Implementation**

```cpp
inline std::string capitalize(std::string s) {
  if (!s.empty())
    s[0] = static_cast<char>(
        std::toupper(static_cast<unsigned char>(s[0])));
  return s;
}

inline std::string title(std::string s) {
  bool word_start = true;
  for (char& c : s) {
    unsigned char u = static_cast<unsigned char>(c);
    if (std::isspace(u)) {
      word_start = true;
    } else if (word_start) {
      c = static_cast<char>(std::toupper(u));
      word_start = false;
    }
  }
  return s;
}
```

## `join` over any range [P2]

**Why:** `join(std::vector<std::string>)` forces materialization; `join`
accepting any range of string-convertibles removes the intermediate vector
for pipeline output.

**Implementation** — needs `<ranges>` (already present); the
`vector<string>` overload keeps winning for the common call

```cpp
template <std::ranges::range R>
std::string join(R const& parts, std::string const& sep) {
  std::string out;
  bool first = true;
  for (auto const& p : parts) {
    if (!first)
      out += sep;
    out += p;
    first = false;
  }
  return out;
}
```

## `chunk` strings [P2]

```
std::vector<std::string> chunk(std::string s, size_t n)
```

**Why:** Formatting fixed-width blocks, splitting hex dumps, wrapping tokens.
Pairs with vec's `chunk` by analogy; byte-based.

**Implementation** — byte-based; `n == 0` yields empty

```cpp
inline std::vector<std::string> chunk(std::string const& s, size_t n) {
  std::vector<std::string> out;
  if (n == 0)
    return out;
  for (size_t i = 0; i < s.size(); i += n)
    out.emplace_back(s.substr(i, n));
  return out;
}
```

## `starts_with` / `ends_with` free wrappers [P2]

**Why:** C++20 `std::string` has these members, but a *free-function* wrapper
keeps `fp::str` chains uniform (`str::starts_with(s, "f") &&` reads like the
rest of the module) and is trivially total.

**Implementation** — total predicates; `std::string` members already do this,
the wrappers keep `fp::str` call sites uniform

```cpp
inline bool starts_with(std::string const& s, std::string const& prefix) {
  return s.rfind(prefix, 0) == 0;
}

inline bool ends_with(std::string const& s, std::string const& suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
```
#pragma once

#include <ostream>

namespace color {

struct code {
  code(const char *str) : str_(str) { }

  friend std::ostream& operator<<(std::ostream &os, const code &c) {
    return os << c.str_;
  }

  static code red, boldred, reset;

private:
  const char *str_;
};

} // end namespace color

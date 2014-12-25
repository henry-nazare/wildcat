#include "stream.h"

#include <iostream>

char stream::next() {
  char c = peek();
  if (c) {
    unsigned col = loc_.get_col(), line = loc_.get_line();
    loc_ = location(c == '\n' ? 1 : col + 1,  c == '\n' ? line + 1 : line);
  }
  return c;
}

char stream::peek() const {
  unsigned col = loc_.get_col(), line = loc_.get_line();
  if (col == lines_.at(line - 1).size() + 1)
    return line == lines_.size() ? '\0' : '\n';
  return lines_.at(line - 1).at(col - 1);
}


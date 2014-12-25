#include "scanner.h"

static bool in_range(char low, char val, char high) {
  return low <= val && val <= high;
}

scanner
  scanner::is_space("space", [](char c) { return c == ' '  || c == '\n'; }),
  scanner::is_word("word", [](char c) { return c && !is_space(c); }),
  scanner::is_ident("identifier", [](char c) {
    return in_range('a', c, 'z') || in_range('A', c, 'Z') ||
           in_range('0', c, '9') || c == '_'; });


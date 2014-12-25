#include "stream.h"
#include "parser.h"

#include <iostream>

static void show_usage(std::ostream &os, char *argv[]) {
  os << argv[0] << " <input file>" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    show_usage(std::cerr, argv);
    exit(1);
  }

  stream s(argv[1]);
  if (s.is_valid()) {
    show_usage(std::cerr, argv);
    std::cerr << "Could not open file: " << argv[1] << std::endl;
    exit(1);
  }

  parser p(s);
  if (!p.parse())
    exit(1);

  return 0;
}


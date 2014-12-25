#pragma once

#include <fstream>
#include <string>
#include <vector>

class stream {
public:
  stream(const char *filename)
    : filename_(filename), loc_(1, 1) {
    std::ifstream input; input.open(filename);
    for (std::string line; std::getline(input, line);)
      lines_.push_back(line);
    input.close();
  }

  class location {
  public:
    location() { }
    location(unsigned col, unsigned line) : col_(col), line_(line) { }

    unsigned get_col()  const { return col_;  }
    unsigned get_line() const { return line_; }

    bool operator<(const location &r) const {
      return get_line() < r.get_line() || get_col() < r.get_col();
    }

    bool operator>(const location &r) const {
      return get_line() > r.get_line() || get_col() > r.get_col();
    }

    bool operator==(const location &r) const {
      return get_line() == r.get_line() && get_col()  == r.get_col();
    }

    bool operator!=(const location &r) const {
      return !(*this == r);
    }

  private:
    unsigned col_, line_;
  };

  const char *get_filename() const { return filename_; }

  location get_loc()       const { return loc_; }
  void     set_loc(location loc) { loc_ = loc;  }

  std::string get_line(unsigned line) const { return lines_.at(line - 1); }

  bool is_valid() { return lines_.empty(); }

  char next();
  char peek() const;

private:
  const char              *filename_;
  std::vector<std::string> lines_;
  location                 loc_;
};


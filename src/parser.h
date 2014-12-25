#pragma once

#include "scanner.h"
#include "stream.h"

#include <set>
#include <string>

class parser {
public:
  parser(stream s) : stream_(s), valid_(true) { }

  class error {
  public:
    error(char expected, char got, stream::location loc);
    error(scanner expected, char got, stream::location loc);
    error(std::string expected, char got, stream::location loc);

    std::string      get_expected() const { return expected_; }
    char             get_got()      const { return got_;      }
    stream::location get_loc()      const { return loc_;      }

    bool operator<(const error &e) const {
      return expected_ < e.expected_ || got_ < e.got_ || loc_ < e.loc_;
    }

  private:
    std::string      expected_;
    char             got_;
    stream::location loc_;
  };

  stream &get_stream() { return stream_; }

  bool is_valid()            const { return valid_;  }
  void set_valid(bool valid)       { valid_ = valid; }

  void add_error(error err);
  void merge_errors(parser &prs);
  void clear_errors();
  // Clears all errors before a specific location in the input stream.
  void clear_errors(stream::location loc);

  void print_error_loc(std::ostream &os, stream::location loc)          const;
  void print_errors(std::ostream &os)                                   const;
  void print_error_unterminated(std::ostream &os, stream::location loc) const;

  // Advances the stream to the next definition.
  void advance();
  bool parse();

  operator bool() const { return is_valid(); }

  template<typename T>
  friend parser& operator>>(parser &prs, T run);
  friend parser& operator<<(parser &prs, parser new_prs);

private:
  stream          stream_;
  bool            valid_;
  std::set<error> errors_;
};


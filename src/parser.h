#pragma once

#include "construct.h"
#include "rtti.h"
#include "scanner.h"
#include "stream.h"

#include <ostream>
#include <set>
#include <stack>
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

  std::set<error> get_errors() const      { return errors_;   }
  void set_errors(std::set<error> errors) { errors_ = errors; }

  void add_error(error err);
  void merge_errors(parser &prs);
  void clear_errors();
  // Clears all errors before a specific location in the input stream.
  void clear_errors(stream::location loc);

  void print_error_loc(std::ostream &os, stream::location loc)          const;
  void print_errors(std::ostream &os)                                   const;
  void print_error_unterminated(std::ostream &os, stream::location loc) const;

  void add_construct(construct *c);

  // Get the construct at the top of the stack and make sure it's of the given
  // type.
  template<typename T>
  T *get_construct() {
    if (T *t = dyn_cast<T>(cons_.top())) {
      cons_.pop();
      return t;
    }
    return nullptr;
  }

  // Get the constructs at the top of the stack with the given type.
  template<typename T>
  std::vector<T*> gather_constructs() {
    std::vector<T*> vec;
    while (!cons_.empty() && isa<T>(cons_.top())) {
      T *t = cast<T>(cons_.top()); cons_.pop();
      vec.push_back(t);
    }
    return vec;
  }

  // Advances the stream to the next definition.
  void advance();
  bool parse();

  operator bool() const { return is_valid(); }

  template<typename T>
  friend parser& operator>>(parser &prs, T run);
  friend parser& operator<<(parser &prs, parser new_prs);

private:
  stream                 stream_;
  bool                   valid_;
  std::set<error>        errors_;
  std::stack<construct*> cons_;
};


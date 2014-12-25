#pragma once

#include <functional>

class scanner {
public:
  scanner(const char *id, std::function<bool(char)> fn)
    : id_(id), fn_(fn) {
  }

  const char *get_id() const { return id_; }

  bool operator()(char c) const { return fn_(c); }

  static scanner is_space, is_ident, is_word;

private:
  const char *id_;
  std::function<bool(char)> fn_;
};


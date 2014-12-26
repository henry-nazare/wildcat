#pragma once

#include <functional>
#include <ostream>
#include <string>
#include <vector>

class construct {
protected:
  enum class type {
    ID, WORD, BODY, DEF,
    TYPE_ID, TYPE_COMPOUND, TYPE_LIST, TYPE_FN,
    ARG_ID, ARG_COMPOUND, ARG_LIST
  };

  construct(type ty) : ty_(ty) { }

public:
  static bool classof(const construct*) { return true; }

  friend std::ostream& operator<<(std::ostream &os, const construct &cons);

  type get_ty() const { return ty_; }

private:
  virtual void print(std::ostream &os) const = 0;

  type ty_;
};

template<typename T>
class construct_vec : public construct {
public:
  construct_vec(construct::type ty, std::vector<T*> list)
    : construct(ty), list_(list) {
  }

  std::vector<T*> get_list() const { return list_; }

private:
  virtual const char *get_ty_str() const = 0;

  virtual void print(std::ostream &os) const {
    os << "[" << get_ty_str() << ", ";
    for (auto it = list_.begin(), e = list_.end(); it != e; ++it)
      os << **it << (std::next(it) != e ? ", " : "]");
  }

  std::vector<T*> list_;
};

class construct_string : public construct {
public:
  construct_string(construct::type ty, std::string str)
    : construct(ty), str_(str) {
  }

  std::string get_str() const { return str_; }

private:
  virtual const char *get_ty_str() const = 0;

  virtual void print(std::ostream &os) const;

  std::string str_;
};

class construct_id : public construct_string {
public:
  construct_id(std::string str) : construct_string(type::ID, str) { }

  static bool classof(const construct *c) {
    return c->get_ty() == type::ID;
  }

private:
  virtual const char *get_ty_str() const { return "id"; }
};

class construct_word : public construct_string {
public:
  construct_word(std::string str) : construct_string(type::WORD, str) { }

  static bool classof(const construct *c) {
    return c->get_ty() == type::WORD;
  }

private:
  virtual const char *get_ty_str() const { return "word"; }
};

class construct_body : public construct_vec<construct_word> {
public:
  construct_body(std::vector<construct_word*> list)
    : construct_vec(type::BODY, list) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::BODY;
  }

private:
  virtual const char *get_ty_str() const { return "body"; }
};

class construct_type_id : public construct_string {
public:
  construct_type_id(std::string str)
    : construct_string(type::TYPE_ID, str) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::TYPE_ID;
  }

private:
  virtual const char *get_ty_str() const { return "type_id"; }
};

class construct_type_compound
  : public construct_vec<construct_type_id> {
public:
  construct_type_compound(std::vector<construct_type_id*> list)
    : construct_vec(type::TYPE_COMPOUND, list) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::TYPE_COMPOUND;
  }

private:
  virtual const char *get_ty_str() const { return "type_compound"; }
};

class construct_type_list
  : public construct_vec<construct_type_compound> {
public:
  construct_type_list(std::vector<construct_type_compound*> list)
    : construct_vec(type::TYPE_LIST, list) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::TYPE_LIST;
  }

private:
  virtual const char *get_ty_str() const { return "type_list"; }
};

class construct_type_fn : public construct {
public:
  construct_type_fn(construct_type_list *inp, construct_type_compound *out)
    : construct(type::TYPE_FN), inp_(inp), out_(out) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::TYPE_FN;
  }

  construct_type_list     *get_inp() const { return inp_; }
  construct_type_compound *get_out() const { return out_; }

private:
  virtual void print(std::ostream &os) const;

  construct_type_list     *inp_;
  construct_type_compound *out_;
};

class construct_arg_id : public construct_string {
public:
  construct_arg_id(std::string str) : construct_string(type::ARG_ID, str) { }

  static bool classof(const construct *c) {
    return c->get_ty() == type::ARG_ID;
  }

private:
  virtual const char *get_ty_str() const { return "arg_id"; }
};

class construct_arg_compound
  : public construct_vec<construct_arg_id> {
public:
  construct_arg_compound(std::vector<construct_arg_id*> list)
    : construct_vec(type::ARG_COMPOUND, list) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::ARG_COMPOUND;
  }

private:
  virtual const char *get_ty_str() const { return "arg_compound"; }
};

class construct_arg_list
  : public construct_vec<construct_arg_compound> {
public:
  construct_arg_list(std::vector<construct_arg_compound*> list)
    : construct_vec(type::ARG_LIST, list) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::ARG_LIST;
  }

private:
  virtual const char *get_ty_str() const { return "arg_list"; }
};

class construct_def : public construct {
public:
  construct_def(construct_type_fn *type, construct_arg_list *args,
                construct_body *body)
    : construct(type::DEF), type_(type), args_(args), body_(body) {
  }

  static bool classof(const construct *c) {
    return c->get_ty() == type::DEF;
  }

  construct_type_fn  *get_type() const { return type_; }
  construct_arg_list *get_args() const { return args_; }
  construct_body     *get_body() const { return body_; }

private:
  virtual void print(std::ostream &os) const;

  construct_type_fn  *type_;
  construct_arg_list *args_;
  construct_body     *body_;
};


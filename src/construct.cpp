#include "construct.h"

std::ostream& operator<<(std::ostream &os, const construct &cons) {
  cons.print(os);
  return os;
}

void construct_string::print(std::ostream &os) const {
  os << "[" << get_ty_str() << ", " << str_ << "]";
}

void construct_def::print(std::ostream &os) const {
  os << "[def, " << *type_ << ", " << *args_ << ", " << *body_ << "]";
}

void construct_type_fn::print(std::ostream &os) const {
  os << "[type_fn, " << *inp_ << ", " << *out_ << "]";
}


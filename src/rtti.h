#pragma once

#include <cassert>

template <typename T, typename U>
bool isa(U *val) {
  return T::classof(val);
}

template <typename T, typename U>
T *cast(U *val) {
  assert(isa<T>(val) && "Invalid cast");
  return static_cast<T*>(val);
}

template <typename T, typename U>
T *dyn_cast(U *val) {
  return isa<T>(val) ? cast<T>(val) : nullptr;
}


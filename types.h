#pragma once

#include <string>

enum class PrimTypes {
  INT,
  VOID,
  ARRAY,
  STRUCT,
};

class Types {
public:
  PrimTypes ptype;

  virtual bool compatible(Types *that) { return this->ptype == that->ptype; }
};

class IntTypes : public Types {
public:
  int size;
  IntTypes(int size = 0) {
    this->size = size;
    this->ptype = PrimTypes::INT;
  }
};

class VoidTypes : public Types {
public:
  VoidTypes() { this->ptype = PrimTypes::VOID; }
};

class ArrayTypes : public Types {
public:
  Types *base;
  ArrayTypes(Types *base) {
    this->base = base;
    this->ptype = PrimTypes::ARRAY;
  }

  virtual bool compatible(Types *that) {
    if (that->ptype != PrimTypes::ARRAY)
      return false;
    ArrayTypes *t = (ArrayTypes *)that;
    return base->compatible(t->base);
  };
};

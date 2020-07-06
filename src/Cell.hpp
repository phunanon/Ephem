#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <immer/vector.hpp>
#include "Enums.hpp"
using namespace std;

struct Cell;

union Data {
  void*    ptr;
  Cell*    cell;
  bool     tru;
  Op       op;
  uint8_t  u08;
  char     s08;
  uint16_t u16;
  int16_t  s16;
  uint32_t u32 = 0;
  int32_t  s32;
  float    d32;
};

class Value {
  uint16_t counter = 0;
  void chooseCounter();
public:
  Data data;
  Type type = T_N;

  Value ();
  Value (const Value&);
  Value (Data, Type);
  Value& operator= (const Value&);
  ~Value ();

  uint8_t  size ();
  bool     tru  ();
  Op       op   ();
  void*    ptr  ();
  uint8_t  u08  ();
  char     s08  ();
  uint16_t u16  ();
  int16_t  s16  ();
  uint32_t u32  ();
  int32_t  s32  ();
  float    d32  ();
  string   str  ();
  Cell*    cell ();
  string   toStr();
};

immer::vector<Value>* vec (Value&);

struct Cell {
  Value value;
  Cell* next = nullptr;
  ~Cell ();
};

struct Range {
  int32_t start;
  int32_t end;
  int32_t next;
  Range (int32_t _start, int32_t _end) :
    start(_start), end(_end), next(_start) {}
};
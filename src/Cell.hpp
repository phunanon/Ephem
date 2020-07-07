#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <immer/vector.hpp>
#include "Enums.hpp"
using namespace std;

struct Cell;
class Lizt;

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
  refnum _ref = 0;
  Data _data = Data{};
  Type _type = T_N;

public:
  Value ();
  Value (Data, Type);
  Value (const Value&);
  Value& operator= (const Value&);
  ~Value ();

  void     kill ();
  Data     data ();
  Type     type ();
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
  Lizt*    lizt ();
};

immer::vector<Value>* vec (Value&);

struct Cell {
  Value value;
  Cell* next = nullptr;
  ~Cell ();
};



enum LiztT : uint8_t {
  P_Vec, P_Cycle, P_Range, P_Map, P_Take
};

//P_Vec   _state is vector<Value>*
//P_Cycle _state is vector<Value>*
//P_Range _state is Range*
//P_Map   _state is Map*
//P_Take  _state is Take*
class Lizt {
public:
  struct Range {
    const int32_t from = 0; //
    const int32_t to   = 0; // Equal for infinite
    const int32_t step = 0;
  };
  struct Map {
    vector<Lizt*> sources;
    Cell* head;
    ~Map ();
  };
  struct Take {
    Lizt*   lizt;
    int32_t skip;
    int32_t take; //Negative for infinite
  };

  refnum ref;
  LiztT type;
  lztlen len;
  void* config;

  Lizt (const Lizt&);
  Lizt& operator= (const Lizt&);

  ~Lizt ();
  static Lizt* list  (Value);
  static Lizt* take  (Take);
  static Lizt* range (Range);
  static Lizt* cycle (vector<Value>);
  static Lizt* map   (Cell*, vector<Lizt*>);
  bool isInf ();

private:

  Lizt (LiztT, lztlen, void*);
};
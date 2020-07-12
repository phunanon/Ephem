#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>
#include "Enums.hpp"
using namespace std;

struct Cell;
class Lizt;

union Data {
  void*    ptr; Cell*    cell;
  bool     tru;
  Op       op;
  uint8_t  u08; char     s08;
  uint32_t u32; int32_t  s32 = 0;
  float    d32;
  fid      fID;
};

class Value {
  void setRef ();
  refnum _ref = 0;
  Data _data = Data{};
  Type _type = T_N;

public:
  Value () {}
  Value (Data, Type);
  Value (const Value&);
  Value& operator= (const Value&);
  ~Value ();

  void     kill ();
  Data     data () { return _data; }
  Type     type () { return _type; }
  void*    ptr  () { return _data.ptr; }
  uint8_t  u08  () { return _data.u08; }
  char     s08  () { return _data.s08; }
  uint32_t u32  () { return _data.u32; }
  int32_t  s32  () { return _data.s32; }
  float    d32  () { return _data.d32; }
  fid      func () { return _data.fID; }
  string   str  () { return *(string*)_data.ptr; }
  Cell*    cell () { return (Cell*)_data.ptr; }
  Lizt*    lizt () { return (Lizt*)_data.ptr; }
  //Coercions & information
  uint8_t  size () {
    switch (_type) {
      case T_Op: case T_U08: case T_S08: case T_Bool: return 1;
      case T_U32: case T_S32: case T_D32: return 4;
      default: return 0; //TODO add more
    }
  }
  bool     tru  () { return _type == T_Bool ? _data.tru : _type != T_N; };
  Op       op   () { return _type == T_Op ? _data.op : O_None; }
  uint32_t u32c () { 
    switch (_type) {
      case T_U08: return u08(); case T_S08: return s08();
      case T_S32: return s32(); case T_D32: return d32();
      default: return u32();
    }
  }
  int32_t s32c () { 
    switch (_type) {
      case T_U08: return u08(); case T_S08: return s08();
      case T_U32: return u32(); case T_D32: return d32();
      default: return s32();
    }
  }
  float    d32c () { return _type == T_D32 ? d32() : (float)s32(); }
  bool     hasSign () { return _type == T_S08 || _type == T_S32 || _type == T_D32; }
};

immer::vector<Value>* vec (Value&);

struct Cell {
  Value val;
  Cell* next = nullptr;
  ~Cell ();
  static bool checkMemLeak();
};



enum LiztT : uint8_t {
  P_Vec, P_Take, P_Range, P_Cycle, P_Emit,
  P_Map
};

class Lizt {
public:
  struct Range {
    const int32_t from = 0; //
    const int32_t to   = 0; // Equal for infinite
    const int32_t step = 0;
  };
  struct Take {
    Lizt*   lizt;
    int32_t skip;
    int32_t take; //Negative for infinite
    ~Take ();
  };
  struct Map {
    vector<Lizt*> sources;
    Cell* head;
    ~Map ();
  };

  refnum ref;
  LiztT type;
  veclen len;
  //Config types:
  //  P_Vec:vector<Value>* P_Cycle:vector<Value>* P_Range:Range*
  //  P_Map:Map* P_Take:Take* P_Repeat:Value
  void* config;

  Lizt (const Lizt&);
  Lizt& operator= (const Lizt&);

  ~Lizt ();
  static Lizt* list  (Value);
  static Lizt* take  (Take*);
  static Lizt* range (Range);
  static Lizt* cycle (vector<Value>);
  static Lizt* emit  (Value, veclen);
  static Lizt* map   (Cell*, vector<Lizt*>);
  static veclen length (Value&); 
  bool isInf ();

private:

  Lizt (LiztT, veclen, void*);
};
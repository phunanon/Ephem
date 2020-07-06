#include "Cell.hpp"

#define NUM_OBJ 1024
uint8_t counters[NUM_OBJ] = {0};

void Value::chooseCounter () {
  while (counters[counter] && counter < NUM_OBJ)
    ++counter;
  counters[counter] = 1;
}

Value::Value () {}

Value::Value (const Value& obj) {
  data = obj.data;
  type = obj.type;
  counter = obj.counter;
  ++counters[counter];
}
Value& Value::operator= (const Value& obj) {
  data = obj.data;
  type = obj.type;
  counter = obj.counter;
  ++counters[counter];
  return *this;
}
Value::Value (Data d, Type t) {
  data = d;
  type = t;
  chooseCounter();
}

Value::~Value () {
  if (!counters[counter]) return;
  if (--counters[counter]) return;
//if (type == T_Cell || type == T_Str || type == T_Lamb || type == T_Vec)
//  printf("haha %d\n", type);
  switch (type) {
    case T_Cell: delete cell();            break;
    case T_Lamb: delete cell();            break;
    case T_Str:  delete (string*)data.ptr; break;
    case T_Vec:  delete (immer::vector<Value>*)data.ptr; break;
  }
}


uint8_t Value::size () {
  switch (type) {
    case T_U08: return 1;
    case T_U32: case T_S32: case T_D32: return 4;
    //TODO
  }
  return 0;
}

bool Value::tru () {
  return data.tru && type != T_N;
}
Op Value::op () {
  return type == T_Op ? data.op : O_None;
}
void*    Value::ptr () { return data.ptr; }
uint8_t  Value::u08 () { return data.u08; }
char     Value::s08 () { return data.s08; }
uint16_t Value::u16 () { return data.u16; }
int16_t  Value::s16 () { return data.s16; }
uint32_t Value::u32 () { return data.u32; }
int32_t  Value::s32 () { return data.s32; }
float    Value::d32 () { return data.d32; }
string   Value::str () { return *(string*)data.ptr; }
Cell*    Value::cell() { return (Cell*)data.ptr; }

string Value::toStr () {
  switch (type) {
    case T_N:   return string("N");
    case T_U08: return to_string(u08());
    case T_S08: return string(1, s08());
    case T_U32: return to_string(u32());
    case T_D32: return to_string(d32());
    case T_Str: return str();
    case T_Vec: {
      auto v = *vec(*this);
      auto vLen = v.size();
      if (!vLen) return "[]";
      string vecStr = ((Value)v[0]).toStr();
      for (veclen i = 1; i < vLen; ++i)
        vecStr += " " + ((Value)v[i]).toStr();
      return "["+ vecStr +"]";
    }
    //TODO
  }
  return string("?");
}

immer::vector<Value>* vec (Value &v) {
  return (immer::vector<Value>*)v.ptr();
}

Cell::~Cell () {
  delete next;
};

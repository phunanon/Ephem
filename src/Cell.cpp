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
    case T_Lizt: delete (Lizt*)data.ptr;   break;
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
Lizt*    Value::lizt() { return (Lizt*)data.ptr; }
Cell*    Value::cell() { return (Cell*)data.ptr; }


immer::vector<Value>* vec (Value &v) {
  return (immer::vector<Value>*)v.ptr();
}

Cell::~Cell () {
  delete next;
}



//// Lizt

/// C'tor & D'tor

Lizt::Lizt (LiztT _type, void* _state)
  : type(_type), state(_state) {}

Lizt::~Lizt () {
  switch (type) {
    case P_Vec:
      delete (queue<Value>*)state;
      break;
    case P_Range:
      delete (Range*)state;
      break;
    case P_Map:
      delete (Map*)state;
      break;
  }
}

Lizt::Map::~Map () {
  for (auto s : sources)
    delete s;
}

/// Factories

//Accepts a Value of any type and converts it to a Lizt.
//  If the Value is not a T_Vec or T_Lizt it returns a P_Cycle.
Lizt* Lizt::list (Value v) {
  switch (v.type) {
    case T_Lizt: {
      auto lizt = v.lizt();
      v.data.ptr = nullptr;
      return lizt;
    }
    case T_Vec: {
      auto vect = vec(v);
      auto q = new queue<Value>();
      for (Value val : *vect)
        q->push(val);
      return new Lizt(P_Vec, q);
    }
  }
  return cycle({v});
}

Lizt* Lizt::range (Range range) {
  return new Lizt(P_Range, new Range(range));
}

Lizt* Lizt::cycle (vector<Value> v) {
  return new Lizt(P_Cycle, new Cycle{v});
}

Lizt* Lizt::map (Cell* head, vector<Lizt*> sources) {
  return new Lizt(P_Map, new Map{sources, head});
}

/// Methods

bool Lizt::isEmpty () {
  switch (type) {
    case P_Vec:
      return !((queue<Value>*)state)->size();
    case P_Range: {
      auto r = (Range*)state;
      return r->step ? r->next >= r->to : false;
    }
    case P_Map: {
      auto m = (Lizt::Map*)state;
      for (argnum v = 0, vLen = m->sources.size(); v < vLen; ++v)
        if (m->sources[v]->isEmpty())
          return true;
    }
  }
  return false;
}

bool Lizt::isInfinite () {
  if (type == P_Range)
    return !((Range*)state)->step;
  if (type == P_Cycle)
    return true;
  if (type == P_Map) {
    auto m = (Lizt::Map*)state;
    for (argnum v = 0, vLen = m->sources.size(); v < vLen; ++v)
      if (!m->sources[v]->isInfinite())
        return false;
    return true;
  }
  return false;
}

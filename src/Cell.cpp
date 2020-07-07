#include "Cell.hpp"

#define NUM_OBJ (refnum)2048
static uint8_t refs[NUM_OBJ] = {0};

static refnum newRef () {
  refnum ref = 0;
  while (refs[ref] && ref < NUM_OBJ)
    ++ref;
  return ref;
}

Value::Value () {}

Value::Value (Data d, Type t) : _data(d), _type(t) {
  refs[_ref = newRef()] = 1;
}
Value::Value (const Value& obj)
  : _ref(obj._ref), _data(obj._data), _type(obj._type) {
  ++refs[_ref];
}
Value& Value::operator= (const Value& obj) {
  _data = obj._data;
  _type = obj._type;
  ++refs[_ref = obj._ref];
  return *this;
}

Value::~Value () {
  if (!refs[_ref]) return;
  if (--refs[_ref]) return;
//if (type == T_Cell || type == T_Str || type == T_Lamb || type == T_Vec)
//  printf("haha %d\n", type);
  switch (_type) {
    case T_Cell: delete cell(); break;
    case T_Lamb: delete cell(); break;
    case T_Str:  delete (string*)_data.ptr; break;
    case T_Vec:  delete (immer::vector<Value>*)_data.ptr; break;
    case T_Lizt: delete (Lizt*)_data.ptr; break;
  }
}


void Value::kill () {
  _data = Data{};
  _type = T_N;
}
Data Value::data () { return _data; }
Type Value::type () { return _type; }

uint8_t Value::size () {
  switch (_type) {
    case T_U08: return 1;
    case T_U32: case T_S32: case T_D32: return 4;
    //TODO
  }
  return 0;
}

bool Value::tru () {
  return _data.tru && _type != T_N;
}
Op Value::op () {
  return _type == T_Op ? _data.op : O_None;
}
void*    Value::ptr () { return _data.ptr; }
uint8_t  Value::u08 () { return _data.u08; }
char     Value::s08 () { return _data.s08; }
uint16_t Value::u16 () { return _data.u16; }
int16_t  Value::s16 () { return _data.s16; }
uint32_t Value::u32 () { return _data.u32; }
int32_t  Value::s32 () { return _data.s32; }
float    Value::d32 () { return _data.d32; }
string   Value::str () { return *(string*)_data.ptr; }
Lizt*    Value::lizt() { return (Lizt*)_data.ptr; }
Cell*    Value::cell() { return (Cell*)_data.ptr; }


immer::vector<Value>* vec (Value &v) {
  return (immer::vector<Value>*)v.ptr();
}

Cell::~Cell () {
  delete next;
}



//// Lizt

/// C'tor, D'tor, Copies

Lizt::Lizt (LiztT _type, void* _state) : type(_type), state(_state) {
  refs[ref = newRef()] = 1;
}
Lizt::Lizt (const Lizt& obj) {
  type = obj.type;
  state = obj.state;
  ++refs[ref = obj.ref];
}
Lizt& Lizt::operator= (const Lizt& obj) {
  type = obj.type;
  state = obj.state;
  ++refs[ref = obj.ref];
  return *this;
}

Lizt::~Lizt () {
  if (!refs[ref]) return;
  if (--refs[ref]) return;
  switch (type) {
    case P_Vec:   delete (queue<Value>*)state; break;
    case P_Cycle: delete (Cycle*)state;        break;
    case P_Range: delete (Range*)state;        break;
    case P_Map:   delete (Map*)state;          break;
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
  switch (v.type()) {
    case T_Lizt: 
      return new Lizt(*v.lizt());
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

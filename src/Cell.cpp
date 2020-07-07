#include "Cell.hpp"
#include <limits>

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

Lizt::Lizt (LiztT _type, lztlen _len, void* _state)
  : type(_type), len(_len), config(_state) {
  refs[ref = newRef()] = 1;
}
Lizt::Lizt (const Lizt& obj) {
  type = obj.type;
  len = obj.len;
  config = obj.config;
  ++refs[ref = obj.ref];
}
Lizt& Lizt::operator= (const Lizt& obj) {
  type = obj.type;
  len = obj.len;
  config = obj.config;
  ++refs[ref = obj.ref];
  return *this;
}

Lizt::~Lizt () {
  if (!refs[ref]) return;
  if (--refs[ref]) return;
  switch (type) {
    case P_Vec:   delete (vector<Value>*)config; break;
    case P_Cycle: delete (vector<Value>*)config; break;
    case P_Range: delete (Range*)config;         break;
    case P_Map:   delete (Map*)config;           break;
    case P_Take:  delete (Take*)config;          break;
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
      auto iVect = vec(v);
      auto mVect = new vector<Value>();
      for (Value val : *iVect)
        mVect->push_back(val);
      return new Lizt(P_Vec, mVect->size(), mVect);
    }
  }
  return cycle({v});
}

Lizt* Lizt::take (Take take) {
  lztlen len = take.lizt->len;
  if (len != -1) len = take.take;
  return new Lizt(P_Take, len, new Take(take));
}

Lizt* Lizt::range (Range range) {
  lztlen len = range.to > 0 ? range.to - range.from : range.from - range.to;
  if (!len) len = -1;
  return new Lizt(P_Range, len, new Range(range));
}

Lizt* Lizt::cycle (vector<Value> v) {
  return new Lizt(P_Cycle, -1, new vector<Value>{v});
}

Lizt* Lizt::map (Cell* head, vector<Lizt*> sources) {
  //Check if all are infinite
  lztlen smallest, maximum;
  smallest = maximum = numeric_limits<lztlen>::max();
  for (argnum v = 0, vLen = sources.size(); v < vLen; ++v)
    if (!sources[v]->isInf())
      if (sources[v]->len < smallest)
        smallest = sources[v]->len;
  if (smallest == maximum)
    smallest = -1;
  return new Lizt(P_Map, smallest, new Map{sources, head});
}

/// Methods

bool Lizt::isInf () {
  return len == -1;
}
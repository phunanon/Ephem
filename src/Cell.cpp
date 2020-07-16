#include "Cell.hpp"
#include <limits>

static uint8_t refs[NUM_OBJ] = {0};
refnum leftmostRef = 1;

static refnum newRef () {
  refnum ref = leftmostRef++;
  while (refs[ref] && ref < NUM_OBJ)
    ++ref;
  return ref;
}

void Value::setRef () {
  if (_type == T_Cell || _type == T_Lamb || _type == T_Str || _type == T_Vec || _type == T_Lizt)
    refs[_ref = newRef()] = 1;
}

Value::Value (Data d, Type t) : _data(d), _type(t) {
  setRef();
}
Value::Value (const Value& obj)
  : _ref(obj._ref), _data(obj._data), _type(obj._type) {
  if (_ref) ++refs[_ref];
}
Value& Value::operator= (const Value& obj) {
  this->~Value();
  _data = obj._data;
  _type = obj._type;
  _ref = obj._ref;
  if (_ref) ++refs[_ref];
  return *this;
}

Value::~Value () {
  if (!_ref || !refs[_ref] || --refs[_ref]) return;
//if (type == T_Cell || type == T_Str || type == T_Lamb || type == T_Vec)
//  printf("haha %d\n", type);
  switch (_type) {
    case T_Cell: delete cell(); break;
    case T_Lamb: delete cell(); break;
    case T_Str:  delete (string*)_data.ptr; break;
    case T_Vec:  delete (immer::vector<Value>*)_data.ptr; break;
    case T_Lizt: delete (Lizt*)_data.ptr; break;
  }
  if (_ref < leftmostRef)
    leftmostRef = _ref;
}


void Value::kill () {
  _data = Data{};
  _type = T_N;
}


immer::vector<Value>* vec (Value &v) {
  return (immer::vector<Value>*)v.ptr();
}


Cell::~Cell () {
  delete next;
}

bool Cell::checkMemLeak () {
  refnum ref = 0;
  while (ref < NUM_OBJ && !refs[ref++]);
  return ref != NUM_OBJ;
}



//// Lizt

/// C'tor, D'tor, Copies

Lizt::Lizt (LiztT _type, veclen _len, void* _state)
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
  if (!refs[ref] || --refs[ref]) return;
  switch (type) {
    case P_Vec:   delete (vector<Value>*)config; break;
    case P_Take:  delete (Take*)config;          break;
    case P_Range: delete (Range*)config;         break;
    case P_Cycle: delete (vector<Value>*)config; break;
    case P_Emit:  delete (Value*)config;         break; 
    case P_Map:   delete (Map*)config;           break;
  }
  if (ref < leftmostRef)
    leftmostRef = ref;
}

Lizt::Map::~Map () {
  delete head;
  for (auto s : sources)
    delete s;
}

Lizt::Take::~Take () {
  delete lizt;
}

/// Factories

//Accepts a Value of any type and converts it to a Lizt.
//  If the Value is not a T_Vec or T_Lizt it returns a P_Emit.
Lizt* Lizt::list (Value v) {
  if (v.type() == T_Lizt)
    return new Lizt(*v.lizt());
  if (v.type() == T_Vec) {
    auto iVect = vec(v);
    auto mVect = new vector<Value>();
    for (Value val : *iVect)
      mVect->push_back(val);
    return new Lizt(P_Vec, mVect->size(), mVect);
  }
  return emit(v, -1);
}

Lizt* Lizt::take (Take* take) {
  veclen len = take->take != -1 ? take->take : take->lizt->len;
  return new Lizt(P_Take, len, take);
}

Lizt* Lizt::range (Range range) {
  veclen len = range.to > 0 ? range.to - range.from : range.from - range.to;
  if (len) len /= abs(range.step);
  if (!len) len = -1;
  return new Lizt(P_Range, len, new Range(range));
}

Lizt* Lizt::cycle (vector<Value> v) {
  return new Lizt(P_Cycle, -1, new vector<Value>{v});
}

Lizt* Lizt::emit (Value v, veclen len) {
  return new Lizt(P_Emit, len, new Value(v));
}

Lizt* Lizt::map (Cell* head, vector<Lizt*> sources) {
  //Check if all are infinite
  veclen smallest, maximum;
  smallest = maximum = numeric_limits<veclen>::max();
  for (argnum v = 0, vLen = sources.size(); v < vLen; ++v)
    if (!sources[v]->isInf())
      if (sources[v]->len < smallest)
        smallest = sources[v]->len;
  if (smallest == maximum)
    smallest = -1;
  return new Lizt(P_Map, smallest, new Map{sources, head});
}

/// Methods and non-factory statics

veclen Lizt::length (Value &v) {
  if (v.type() == T_Vec)
    return vec(v)->size();
  if (v.type() == T_Lizt)
    return v.lizt()->len;
  return 0;
}

bool Lizt::isInf () {
  return len == -1;
}
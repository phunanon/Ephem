#include "EVM.hpp"
#include <cstdint>
#include <cstring>
#include <cmath>

argnum numArgs (Cell* a) {
  if (!a) return 0;
  argnum num = 1;
  while ((a = a->next)) ++num;
  return num;
}

bool isCallType (Cell* a) {
  Type t = a->value.type();
  return t == T_Lamb || t == T_Op || t == T_Func;
}

template <class T>
T hcpy (T* ptr) {
  T cpy = *ptr;
  delete ptr;
  return cpy;
}



EVM::~EVM () {
  for (auto f : funcs)
    removeFunc(f.first);
}

void EVM::addFunc (fid id, vector<Cell*> cells) {
  removeFunc(id);
  funcs[id] = cells;
}

void EVM::removeFunc (fid id) {
  if (funcs.find(id) != funcs.end())
    for (auto cell : funcs[id])
      delete cell;
  funcs.erase(id);
}


Value EVM::exeFunc (fid id, Cell* params) {
  if (funcs.find(id) == funcs.end())
    return Value();
  Value ret;
  for (auto cell : funcs[id])
    ret = eval(cell, params);
  return ret;
}


//Returns value after traversal across cell->next, or nil
Value EVM::valAt (Cell* a, argnum by) {
  a = cellAt(a, by);
  return a ? a->value : Value();
}

//Returns Cell* after traversal across cell->next, or nullptr
Cell* EVM::cellAt (Cell* a, argnum by) {
  if (by++)
    while (--by && a->next)
      a = a->next;
  else return a;
  return by ? nullptr : a;
}


Value EVM::o_Math (Cell* a, Op op) {
  Value firstVal = a->value;
  a = a->next;
  const Type resultT = firstVal.type();
  const bool isFloat = resultT == T_D32;
  uint32_t iResult;
  {
    uint32_t d = firstVal.u32();
    memcpy(&iResult, &d, firstVal.size());
  }
  float fResult = *(float*)(&iResult);
  while (a) {
    Value v = a->value;
    uint32_t iNext = 0;
    {
      uint32_t d = v.u32();
      memcpy(&iNext, &d, v.size());
    }
    if (isFloat) {
      float fNext = v.type() == T_D32 ? *(float*)(&iNext) : (float)iNext;
      switch (op) {
        case O_Add: fResult += fNext; break;
        case O_Sub: fResult -= fNext; break;
        case O_Mul: fResult *= fNext; break;
        case O_Div: fResult /= fNext; break;
        case O_Mod: fResult = fmod(fResult, fNext); break;
      }
    } else {
      if (v.type() == T_D32) iNext = *(float*)(&iNext);
      switch (op) {
        case O_Add: iResult += iNext; break;
        case O_Sub: iResult -= iNext; break;
        case O_Mul: iResult *= iNext; break;
        case O_Div: iResult /= iNext; break;
        case O_Mod: iResult %= iNext; break;
      }
    }
    a = a->next;
  }
  if (isFloat)
    return Value(Data{.d32 = fResult}, resultT);
  return Value(Data{.u32 = iResult}, resultT);
}


bool areEqual (Value v0, Value v1) {
  return v0.u32() == v1.u32();
}

bool EVM::areAlike (Value v0, Value v1) {
  Type type0 = v0.type();
  Type type1 = v1.type();
  //Ensure mutual comparison for floats
  if ((type0 == T_D32 || type1 == T_D32) && (type0 != type1))
    return false;
  //Compare strings
  if (type0 == T_Str || type1 == T_Str) {
    if (type0 != type1) return false; //Mutual comparison only
    return !v0.str().compare(v1.str());
  } else
  //Compare vectors and Lizts
  if ((type0 == T_Vec || type0 == T_Lizt)
    && (type1 == T_Vec || type1 == T_Lizt)) {
    Lizt* lizt0 = Lizt::list(v0);
    Lizt* lizt1 = Lizt::list(v1);
    bool ret = lizt0->len == lizt1->len;
    if (ret)
      for (veclen i = 0, lLen = lizt0->len; i < lLen; ++i)
        if (!areAlike(liztAt(lizt0, i), liztAt(lizt1, i))) {
          ret = false;
          break;
        }
    delete lizt0;
    delete lizt1;
    return ret;
  }
  //Decay into Data equality
  return areEqual(v0, v1);
}

Value EVM::o_Equal (Cell* a, Op op) {
  if (!a) return Value();
  Value compare = a->value;
  //Loop will break early on false comparison
  while ((a = a->next)) {
    Value to = a->value;
    if (op == O_Alike || op == O_NAlike)
      if (areAlike(compare, to) ^ (op == O_Alike))
        break;
    if (op == O_Equal || op == O_NEqual)
      if (areEqual(compare, to) ^ (op == O_Equal))
        break;
    //if (op == O_GThan)
    //  if (!greaterThan(compare, to))
    //    break;
    //For O_GThan, O_LThan, O_GETo, O_LETo, TODO
    compare = to;
  }
  return Value(Data{.tru=!a}, T_Boo);
}


Value EVM::o_Vec (Cell* a) {
  auto vect = immer::vector_transient<Value>();
  while (a) {
    vect.push_back(a->value);
    a = a->next;
  }
  return Value(Data{.ptr=new immer::vector<Value>(vect.persistent())}, T_Vec);
}


//Returns a skip Lizt.
//  e.g. (skip n vec)
Value EVM::o_Skip (Cell* a) {
  if (numArgs(a) != 2) return Value();
  auto take = new Lizt::Take{Lizt::list(a->next->value), a->value.s32(), -1};
  return Value(Data{.ptr=Lizt::take(take)}, T_Lizt);
}


//Returns a skip/take Lizt.
//  e.g. (take n vec) (take n skip vec)
Value EVM::o_Take (Cell* a) {
  argnum n = numArgs(a);
  if (n < 2) return Value();
  auto takeN = a->value.s32();
  auto skipN = n == 3 ? a->next->value.s32() : 0;
  Lizt* lizt = Lizt::list(valAt(a, n == 2 ? 1 : 2));
  if (!lizt->isInf() && skipN + takeN > lizt->len)
    takeN = lizt->len - skipN;
  if (takeN < 0) takeN = 0;
  auto take = new Lizt::Take{lizt, skipN, takeN};
  return Value(Data{.ptr=Lizt::take(take)}, T_Lizt);
}


//Returns a range Lizt.
//  e.g. (range) (range to) (range from to) (range from to step)
Value EVM::o_Range (Cell* a) {
  int32_t from = 0, to = 0, step = a ? 1 : 0;
  auto n = numArgs(a);
  if (n == 1) to = a->value.s32();
  else if (n > 1) {
    from = a->value.s32();
    to = a->next->value.s32();
    if (n == 3)
      step = a->next->next->value.s32();
  }
  if (n != 3 && to < 0)
    step = -1;
  return Value(Data{.ptr=Lizt::range(Lizt::Range{from, to, step})}, T_Lizt);
}


Value EVM::o_Cycle (Cell* a) {
  auto vals = vector<Value>();
  while (a) {
    vals.push_back(a->value);
    a = a->next;
  }
  return Value(Data{.ptr=Lizt::cycle(vals)}, T_Lizt);
}


Value EVM::o_Emit (Cell* a) {
  if (!a) return Value();
  veclen len = a->next ? a->next->value.s32() : -1;
  return Value(Data{.ptr=Lizt::emit(a->value, len)}, T_Lizt);
}


Value EVM::o_Map (Cell* a) {
  if (!isCallType(a)) return Value();
  Cell* head = new Cell{a->value};
  auto vectors = vector<Lizt*>();
  while ((a = a->next))
    vectors.push_back(Lizt::list(a->value));
  return Value(Data{.ptr=Lizt::map(head, vectors)}, T_Lizt);
}


//Returns a filtered T_Vec from a T_Lizt
// e.g. (where f lizt) (where f take lizt) (where f take skip lizt)
Value EVM::o_Where (Cell* a) {
  if (!isCallType(a)) return Value();
  auto n = numArgs(a);
  Lizt lizt = hcpy(Lizt::list(valAt(a, n - 1)));
  if (lizt.isInf()) return Value();
  veclen skipN = n == 4 ? valAt(a, 2).s32() : 0;
  uint   takeN = n >= 3 ? valAt(a, 1).s32() : lizt.len;
  auto list = immer::vector_transient<Value>();
  Cell head = Cell{a->value};
  Cell form = Cell{Value(Data{.cell=&head}, T_Cell)};
  for (veclen i = skipN; i < lizt.len && list.size() < takeN; ++i) {
    Value testVal = liztAt(&lizt, i);
    Cell valCell = Cell{testVal};
    head.next = &valCell;
    Value v = eval(&form);
    if (!v.tru()) continue;
    list.push_back(testVal);
  }
  head.next = nullptr;
  form.value.kill(); //To ensure head isn't deleted
  auto iVec = new immer::vector<Value>(list.persistent());
  return Value(Data{.ptr=iVec}, T_Vec);
}


Value EVM::o_Str (Cell* a) {
  auto str = new string();
  while (a) {
    *str += toStr(a->value);
    a = a->next;
  }
  return Value(Data{.ptr = str}, T_Str);
}


Value EVM::o_Print (Cell* a, bool nl) {
  Value v = o_Str(a);
  printf("%s", v.str().c_str());
  if (nl) printf("\n");
  else fflush(stdout);
  return Value();
}


Value EVM::exeOp (Op op, Cell* a) {
  switch (op) {
    case O_Add: case O_Sub: case O_Mul: case O_Div:
    case O_Mod:
                   return o_Math(a, op);
    case O_Alike: case O_NAlike: case O_Equal: case O_NEqual:
                   return o_Equal(a, op);
    case O_Vec:    return o_Vec(a);
    case O_Skip:   return o_Skip(a);
    case O_Take:   return o_Take(a);
    case O_Range:  return o_Range(a);
    case O_Cycle:  return o_Cycle(a);
    case O_Emit:   return o_Emit(a);
    case O_Map:    return o_Map(a);
    case O_Where:  return o_Where(a);
    case O_Str:    return o_Str(a);
    case O_Print: case O_Prinln:
                   return o_Print(a, op == O_Prinln);
    case O_Val:    return a->value;
    case O_Do: 
      do {
        if (!a->next) return a->value;
      } while ((a = a->next));
  }
  return Value();
}

Value EVM::eval (Cell* a, Cell* p) {
  Type t = a->value.type();
  //if (t == T_Cell || t == T_Lamb) {
  //  //Have lambdas use neighbour as parameter list
  //  if (t == T_Lamb)
  //    p = a->next;
  if (t == T_Cell) {
    //Evaluate all form arguments
    a = a->value.cell();
    Cell head = Cell{eval(a, p)}, *arg = &head;
    //Handle short-circuited forms
    if (head.value.op() == O_If)
      return eval(cellAt(a, eval(a->next, p).tru() ? 2 : 3), p);
    //Continue collecting arguments
    while ((a = a->next)) {
      arg = arg->next ? arg->next : arg;
      arg->next = new Cell{eval(a, p)};
    }
    //... then call the operation/lambda/function
    if (head.value.type() == T_Op)
      return head.value.op()
        ? exeOp(head.value.op(), head.next)
        : head.value; //This can happen with REPL non-form evals(?)
    else
    if (head.value.type() == T_Lamb) {
      Cell lHead = Cell{Value{head.value.data(), T_Cell}};
      auto ret = eval(&lHead, head.next);
      lHead.value.kill();
      return ret;
    } else
    if (head.value.type() == T_Func)
      return exeFunc(head.value.func(), head.next);
  }
  //Return parameter or nil
  if (t == T_Para)
    return p ? valAt(p, a->value.u08()) : Value();
  //TODO: variables
  return a->value;
}



string EVM::toStr (Value v) {
  switch (v.type()) {
    case T_N:   return string("N");
    case T_U08: return to_string(v.u08());
    case T_S08: return string(1, v.s08());
    case T_U32: return to_string(v.u32());
    case T_S32: return to_string(v.s32());
    case T_D32: return to_string(v.d32());
    case T_Boo: return v.tru() ? "T" : "F";
    case T_Str: return v.str();
    case T_Vec: {
      auto vect = *vec(v);
      auto vLen = vect.size();
      if (!vLen) return "[]";
      string vecStr = toStr((Value)vect[0]);
      for (uint i = 1; i < vLen; ++i)
        vecStr += " " + toStr((Value)vect[i]);
      return "["+ vecStr +"]";
    }
    case T_Lizt: return toStr(liztFrom(v.lizt(), 0)); break;
    //TODO
  }
  return string("?");
}

//Returns next value of the lazy list
Value EVM::liztAt (Lizt* l, veclen at) {
  switch (l->type) {
    case LiztT::P_Vec: {
      auto list = (vector<Value>*)l->config;
      return list->at(at);
    }
    case LiztT::P_Take: {
      auto t = (Lizt::Take*)l->config;
      return liztAt(t->lizt, t->skip + at);
    }
    case LiztT::P_Range: {
      auto r = (Lizt::Range*)l->config;
      int32_t n = r->from == r->to ? at : r->from + (at * r->step);
      return Value(Data{.s32=n}, T_S32);
    }
    case LiztT::P_Cycle: {
      auto c = (vector<Value>*)l->config;
      return c->at(at % c->size());
    }
    case LiztT::P_Emit:
      return *(Value*)l->config;
    case LiztT::P_Map: {
      auto m = (Lizt::Map*)l->config;
      Cell args = Cell{liztAt(m->sources[0], at)}, *arg = &args;
      for (argnum v = 1, vLen = m->sources.size(); v < vLen; ++v) {
        arg = arg->next ? arg->next : arg;
        arg->next = new Cell{liztAt(m->sources[v], at)};
      }
      m->head->next = &args;
      //Create a temporary form Cell
      Cell form = Cell{Value(Data{.cell=m->head}, T_Cell)};
      Value v = eval(&form);
      form.value.kill(); //Ensure head is not destroyed with form
      m->head->next = nullptr;
      return v;
    }
  }
  return Value();
}

//Returns remaining Lizt items as T_Vec
Value EVM::liztFrom (Lizt* l, veclen from) {
  if (l->isInf()) return Value();
  auto list = immer::vector_transient<Value>();
  for (auto i = from; i < l->len; ++i)
    list.push_back(liztAt(l, i));
  return Value(Data{.ptr=new immer::vector<Value>(list.persistent())}, T_Vec);
}
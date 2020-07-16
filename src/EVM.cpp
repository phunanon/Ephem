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
  Type t = a->val.type();
  return t == T_Lamb || t == T_Op || t == T_Func;
}

template <class T>
T hcpy (T* ptr) {
  T cpy = *ptr;
  delete ptr;
  return cpy;
}



FuncList::~FuncList () {
  auto idsToRem = ids;
  for (auto id : idsToRem)
    remove(id);
}

int FuncList::funcAt (fid id) {
  for (uint i = 0, iLen = _numFuncs; i < iLen; ++i)
    if (ids[i] == id)
      return i;
  return -1;
}

vector<Cell*>* FuncList::get (fid id) {
  auto idx = funcAt(id);
  return idx != -1 ? &funcs[idx] : nullptr;
}

void FuncList::remove (fid id) {
  auto idx = funcAt(id);
  if (idx == -1) return;
  for (auto cell : funcs[idx])
    delete cell;
  ids.erase(ids.begin() + idx);
  funcs.erase(funcs.begin() + idx);
  --_numFuncs;
}

void FuncList::add (fid id, vector<Cell*> cells) {
  ids.push_back(id);
  funcs.push_back(cells);
  ++_numFuncs;
}

void EVM::addFunc (fid id, vector<Cell*> cells) {
  removeFunc(id);
  funcs.add(id, cells);
}

void EVM::removeFunc (fid id) {
  funcs.remove(id);
}


Value EVM::exeFunc (fid id, Cell* params) {
  auto func = funcs.get(id);
  if (!func) return Value();
  Value ret;
  for (uint i = 0, iLen = func->size(); i < iLen; ++i) {
    ret = eval(func->at(i), params);
    if (recurGarbage) {
      delete recurGarbage;
      recurGarbage = nullptr;
    }
    if (doRecur) {
      doRecur = false;
      i = -1;
      params = recurGarbage = recurArgs;
    }
  }
  return ret;
}


//Returns value after traversal across cell->next, or nil
Value EVM::valAt (Cell* a, argnum by) {
  a = cellAt(a, by);
  return a ? a->val : Value();
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
  const Type t = a->val.type();
  const bool hasSign = a->val.hasSign(), isFloat = t == T_D32, isByte = a->val.size() == 1;
  uint32_t uResult = a->val.u32c();
  int32_t  sResult = a->val.s32c();
  float    dResult = a->val.d32c();
  while ((a = a->next)) {
    if (isFloat)
      switch (op) {
        case O_Add: dResult  += a->val.d32c(); break;
        case O_Sub: dResult  -= a->val.d32c(); break;
        case O_Mul: dResult  *= a->val.d32c(); break;
        case O_Div: dResult  /= a->val.d32c(); break;
        case O_Mod: dResult  = fmod(dResult, a->val.d32c()); break;
        case O_Pow: dResult  = pow(dResult, a->val.d32c()); break;
      }
    else if (hasSign)
      switch (op) {
        case O_Add: sResult  += a->val.s32c(); break;
        case O_Sub: sResult  -= a->val.s32c(); break;
        case O_Mul: sResult  *= a->val.s32c(); break;
        case O_Div: sResult  /= a->val.s32c(); break;
        case O_Mod: sResult  %= a->val.s32c(); break;
        case O_Pow: sResult  = pow(sResult, a->val.s32c()); break;
        case O_BA:  sResult  &= a->val.s32c(); break;
        case O_BO:  sResult  |= a->val.s32c(); break;
        case O_BXO: sResult  ^= a->val.s32c(); break;
        case O_BLS: sResult <<= a->val.s32c(); break;
        case O_BRS: sResult >>= a->val.s32c(); break;
      }
    else
      switch (op) {
        case O_Add: uResult  += a->val.u32c(); break;
        case O_Sub: uResult  -= a->val.u32c(); break;
        case O_Mul: uResult  *= a->val.u32c(); break;
        case O_Div: uResult  /= a->val.u32c(); break;
        case O_Mod: uResult  %= a->val.u32c(); break;
        case O_Pow: uResult  = pow(uResult, a->val.u32c()); break;
        case O_BA:  uResult  &= a->val.u32c(); break;
        case O_BO:  uResult  |= a->val.u32c(); break;
        case O_BXO: uResult  ^= a->val.u32c(); break;
        case O_BLS: uResult <<= a->val.u32c(); break;
        case O_BRS: uResult >>= a->val.u32c(); break;
      }
    if (isByte) sResult &= 0xFF;
  }
  if (isFloat) return Value(Data{.d32=dResult}, t);
  if (hasSign) return Value(Data{.s32=sResult}, t);
  return Value(Data{.u32=uResult}, t);
}

Value o_BN (Value v) {
  switch (v.type()) {
    case T_U08: return Value{Data{.u08=(uint8_t)~v.u08()}, T_U08};
    case T_S08: return Value{Data{.s08=(char)~v.s08()}, T_S08};
    case T_U32: return Value{Data{.u32=~v.u32()}, T_U32};
    case T_S32: return Value{Data{.s32=~v.s32()}, T_S32};
  }
  return Value();
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
  if (type0 == T_Str) //Guaranteed mutual types
    return !v0.str().compare(v1.str());
  else
  //Match infinite Lizt and Nil
  if ((type0 == T_N && type1 == T_Lizt)
   || (type0 == T_Lizt && type1 == T_N))
    return (type0 == T_Lizt ? v0.lizt() : v1.lizt())->isInf();
  else
  //Compare lists by item
  if ((type0 == T_Vec || type0 == T_Lizt)
   && (type1 == T_Vec || type1 == T_Lizt)) {
    Lizt lizt0 = hcpy(Lizt::list(v0));
    Lizt lizt1 = hcpy(Lizt::list(v1));
    if ((lizt0.len != lizt1.len)
     || (lizt0.isInf() && lizt1.isInf()))
      return false;
    for (veclen i = 0, lLen = lizt0.len; i < lLen; ++i)
      if (!areAlike(liztAt(&lizt0, i), liztAt(&lizt1, i)))
        return false;
    return true;
  }
  //Decay into Data equality
  return areEqual(v0, v1);
}

bool numDiff (Value v0, Value v1, bool greater) {
  Type type0 = v0.type();
  Type type1 = v1.type();
  //Compare strings
  if (type0 == T_Str) //Guaranteed mutual types
    return v0.str().compare(v1.str()) == (greater ? 1 : -1);
  //Compare lists by length
  if ((type0 == T_Vec || type0 == T_Lizt)
   && (type1 == T_Vec || type1 == T_Lizt))
    return greater
      ? (Lizt::length(v0) < Lizt::length(v1))
      : (Lizt::length(v0) > Lizt::length(v1));
  //Compare as ints/floats
  return greater
    ? (v0.d32c() < v1.d32c())
    : (v0.d32c() > v1.d32c());
}

Value EVM::o_Equal (Cell* a, Op op) {
  if (!a) return Value();
  Value v0 = a->val;
  //Loop will break early on false comparison
  while ((a = a->next)) {
    Value v1 = a->val;
    if ((v0.type() == T_Str || v1.type() == T_Str) && v0.type() != v1.type())
      break; //Mutual string comparison only
    switch (op) {
      case O_Alike: case O_NAlike:
        if (areAlike(v0, v1) ^ (op == O_Alike))
          goto stopComparing;
        break;
      case O_Equal: case O_NEqual:
        if (areEqual(v0, v1) ^ (op == O_Equal))
          goto stopComparing;
        break;
      case O_GThan: case O_LETo:
        if (numDiff(v0, v1, true) ^ (op == O_GThan))
          goto stopComparing;
        break;
      case O_LThan: case O_GETo:
        if (numDiff(v0, v1, false) ^ (op == O_LThan))
          goto stopComparing;
        break;
    }
    v0 = v1;
  }
  stopComparing: ;
  return Value(Data{.tru=!a}, T_Bool);
}


Value EVM::o_Vec (Cell* a) {
  auto vect = immer::vector_transient<Value>();
  while (a) {
    vect.push_back(a->val);
    a = a->next;
  }
  return Value(Data{.ptr=new immer::vector<Value>(vect.persistent())}, T_Vec);
}


//Returns a skip Lizt.
//  e.g. (skip n vec)
Value EVM::o_Skip (Cell* a) {
  if (numArgs(a) != 2) return Value();
  auto take = new Lizt::Take{Lizt::list(a->next->val), a->val.s32(), -1};
  return Value(Data{.ptr=Lizt::take(take)}, T_Lizt);
}


//Returns a skip/take Lizt.
//  e.g. (take n vec) (take n skip vec)
Value EVM::o_Take (Cell* a) {
  argnum n = numArgs(a);
  if (n < 2) return Value();
  auto takeN = a->val.s32();
  auto skipN = n == 3 ? a->next->val.s32() : 0;
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
  if (n == 1) to = a->val.s32();
  else if (n > 1) {
    from = a->val.s32();
    to = a->next->val.s32();
    if (n == 3)
      step = a->next->next->val.s32();
  }
  if (n != 3 && to < 0)
    step = -1;
  return Value(Data{.ptr=Lizt::range(Lizt::Range{from, to, step})}, T_Lizt);
}


Value EVM::o_Cycle (Cell* a) {
  auto vals = vector<Value>();
  while (a) {
    vals.push_back(a->val);
    a = a->next;
  }
  return Value(Data{.ptr=Lizt::cycle(vals)}, T_Lizt);
}


Value EVM::o_Emit (Cell* a) {
  if (!a) return Value();
  veclen len = a->next ? a->next->val.s32() : -1;
  return Value(Data{.ptr=Lizt::emit(a->val, len)}, T_Lizt);
}


Value EVM::o_Map (Cell* a) {
  if (!isCallType(a)) return Value();
  Cell* head = new Cell{a->val};
  auto vectors = vector<Lizt*>();
  while ((a = a->next))
    vectors.push_back(Lizt::list(a->val));
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
  Cell head = Cell{a->val};
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
  form.val.kill(); //To ensure head isn't deleted
  auto iVec = new immer::vector<Value>(list.persistent());
  return Value(Data{.ptr=iVec}, T_Vec);
}


Value EVM::o_Str (Cell* a) {
  auto str = new string();
  while (a) {
    *str += toStr(a->val);
    a = a->next;
  }
  return Value(Data{.ptr = str}, T_Str);
}


Value EVM::o_Print (Cell* a, bool nl) {
  Value v = o_Str(a);
  env.print(v.str().c_str());
  if (nl) env.print("\n");
  else fflush(stdout);
  return Value();
}


Value EVM::exeOp (Op op, Cell* a) {
  if (O_Add <= op && op <= O_BRS)
    return o_Math(a, op);
  if (O_Alike <= op && op <= O_LETo)
    return o_Equal(a, op);
  switch (op) {
    case O_BN:     return o_BN(a->val);
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

    case O_Not:    return Value{Data{.tru=!a->val.tru()}, T_Bool};
    case O_Val:    return a->val;
    case O_Do:
      do {
        if (!a->next) return a->val;
      } while ((a = a->next));
    case O_RKey: {
      char ch = Env::getKey();
      return ch ? Value{Data{.s08=ch}, T_S08} : Value();
    }
    case O_RStr: {
      string prompt = a ? a->val.str() : "";
      return Value{Data{.ptr=Env::getString(prompt)}, T_Str};
    }
    case O_Sleep: env.sleep(a ? a->val.d32c() * 1000 : 1000); break;
  }
  return Value();
}

Value EVM::eval (Cell* a, Cell* p) {
  if (doRecur) return Value();
  Type t = a->val.type();
  if (t == T_Cell) {
    //Evaluate all form arguments
    a = a->val.cell();
    Cell head = Cell{eval(a, p)}, *arg = &head;
    Op op = head.val.op();
    //Handle short-circuited forms
    if (op) {
      if (op == O_If)
        return eval(cellAt(a, eval(a->next, p).tru() ? 2 : 3), p);
      if (op == O_Or) {
        while ((a = a->next))
          if (auto ret = eval(a, p); ret.tru())
            return ret;
        return Value();
      }
      if (op == O_And) {
        while ((a = a->next))
          if (!eval(a, p).tru())
            return Value{Data{.tru=false}, T_Bool};
        return Value{Data{.tru=true}, T_Bool};
      }
    }
    //Continue collecting arguments
    while ((a = a->next)) {
      arg = arg->next ? arg->next : arg;
      arg->next = new Cell{eval(a, p)};
    }
    //... then call the operation/lambda/function
    if (head.val.type() == T_Op) {
      if (op == O_Recur) {
        doRecur = true;
        recurArgs = head.next ? new Cell{head.next->val} : nullptr;
        head.next = nullptr;
        return Value();
      }
      return op ? exeOp(op, head.next) : head.val;
    } else
    if (head.val.type() == T_Lamb) {
      Cell lHead = Cell{Value{head.val.data(), T_Cell}};
      auto ret = eval(&lHead, head.next);
      lHead.val.kill();
      return ret;
    } else
    if (head.val.type() == T_Func)
      return exeFunc(head.val.func(), head.next);
  }
  //Return parameter or nil
  if (t == T_Para)
    return p ? valAt(p, a->val.u08()) : Value();
  //TODO: variables
  return a->val;
}



string EVM::toStr (Value v) {
  switch (v.type()) {
    case T_N:    return string("N");
    case T_U08:  return to_string(v.u08());
    case T_S08:  return string(1, v.s08());
    case T_U32:  return to_string(v.u32());
    case T_S32:  return to_string(v.s32());
    case T_D32:  return to_string(v.d32());
    case T_Bool: return v.tru() ? "T" : "F";
    case T_Str:  return v.str();
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
      form.val.kill(); //Ensure head is not destroyed with form
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
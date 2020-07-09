#include "EVM.hpp"
#include <cstdint>
#include <cstring>

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
Value EVM::valAt (Cell* a, Cell* p, argnum by) {
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

argnum numArgs (Cell* a) {
  if (!a) return 0;
  argnum num = 1;
  while ((a = a->next)) ++num;
  return num;
}


Value EVM::o_Math (Cell* a, Cell* p, Op op) {
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
      }
    } else {
      if (v.type() == T_D32) iNext = *(float*)(&iNext);
      switch (op) {
        case O_Add: iResult += iNext; break;
        case O_Sub: iResult -= iNext; break;
        case O_Mul: iResult *= iNext; break;
        case O_Div: iResult /= iNext; break;
      }
    }
    a = a->next;
  }
  if (isFloat)
    return Value(Data{.d32 = fResult}, resultT);
  return Value(Data{.u32 = iResult}, resultT);
}


Value EVM::o_Vec (Cell* a, Cell* p) {
  auto vect = immer::vector<Value>();
  while (a) {
    vect = vect.push_back(a->value);
    a = a->next;
  }
  auto vectPtr = new immer::vector<Value>(vect);
  return Value(Data{.ptr=vectPtr}, T_Vec);
}


//Returns a skip Lizt.
//  e.g. (skip n vec)
Value EVM::o_Skip (Cell* a, Cell* p) {
  if (numArgs(a) != 2) return Value();
  auto take = Lizt::Take{Lizt::list(a->next->value), a->value.s32(), -1};
  return Value(Data{.ptr=Lizt::take(take)}, T_Lizt);
}


//Returns a skip/take Lizt.
//  e.g. (take n vec) (take n skip vec)
Value EVM::o_Take (Cell* a, Cell* p) {
  argnum n = numArgs(a);
  if (n < 2) return Value();
  auto takeN = a->value.s32();
  auto skipN = n == 3 ? a->next->value.s32() : 0;
  Lizt* lizt = Lizt::list(n == 2 ? a->next->value : a->next->next->value);
  if (!lizt->isInf() && skipN + takeN > lizt->len)
    takeN = lizt->len - skipN;
  if (takeN < 0) takeN = 0;
  auto take = Lizt::Take{lizt, skipN, takeN};
  return Value(Data{.ptr=Lizt::take(take)}, T_Lizt);
}


//Returns a range Lizt.
//  e.g. (range) (range to) (range from to) (range from to step)
Value EVM::o_Range (Cell* a, Cell* p) {
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


Value EVM::o_Cycle (Cell* a, Cell* p) {
  auto vals = vector<Value>();
  while (a) {
    vals.push_back(a->value);
    a = a->next;
  }
  return Value(Data{.ptr=Lizt::cycle(vals)}, T_Lizt);
}


Value EVM::o_Emit (Cell* a, Cell* p) {
  if (!a) return Value();
  veclen len = a->next ? a->next->value.s32() : -1;
  return Value(Data{.ptr=Lizt::emit(a->value, len)}, T_Lizt);
}


//Ensure function is an op, lambda, or function
Cell* EVM::makeHead (Cell* a, Cell* p) {
  Type t = a->value.type();
  if (t == T_Lamb || t == T_Op || t == T_Func)
    return new Cell{a->value};
  Value v = a->value;
  if (v.type() != T_Lamb && v.type() != T_Op)
    return nullptr;
  return new Cell{v};
}


Value EVM::o_Map (Cell* a, Cell* p) {
  Cell* head = makeHead(a, p);
  if (!head) return Value(); //f wasn't callable
  auto vectors = vector<Lizt*>();
  while ((a = a->next))
    vectors.push_back(Lizt::list(a->value));
  return Value(Data{.ptr=Lizt::map(head, vectors)}, T_Lizt);
}


//Returns a filtered T_Vec from a T_Lizt
// e.g. (where f lizt) (where f take lizt) (where f take skip lizt)
Value EVM::o_Where (Cell* a, Cell* p) {
  Cell* head = makeHead(a, p);
  if (!head) return Value();
  auto n = numArgs(a);
  Lizt* lizt = Lizt::list(valAt(a, p, n - 1));
  if (lizt->isInf()) return Value();
  veclen skipN = n == 4 ? valAt(a, p, 2).s32() : 0;
  uint   takeN = n >= 3 ? valAt(a, p, 1).s32() : lizt->len;
  auto list = immer::vector<Value>();
  for (veclen i = skipN; i < lizt->len; ++i) {
    Value testVal = liztAt(lizt, i);
    Cell valCell = Cell{testVal};
    head->next = &valCell;
    Cell form = Cell{Value(Data{.cell=head}, T_Cell)};
    Value v = eval(&form);
    form.value.kill(); //Ensure head is not destroyed with form
    if (!v.tru()) continue;
    list = list.push_back(testVal);
    if (list.size() == takeN) break;
  }
  head->next = nullptr;
  delete head;
  return Value(Data{.ptr=new immer::vector<Value>(list)}, T_Vec);
}


Value EVM::o_Str (Cell* a, Cell* p) {
  auto str = new string();
  while (a) {
    *str += toStr(a->value);
    a = a->next;
  }
  return Value(Data{.ptr = str}, T_Str);
}


Value EVM::o_Print (Cell* a, Cell* p, bool nl) {
  Value v = o_Str(a, p);
  printf("%s", v.str().c_str());
  if (nl) printf("\n");
  else fflush(stdout);
  return Value();
}


Value EVM::o_Val (Cell* a, Cell* p) {
  return a->value;
}


Value EVM::exeOp (Op op, Cell* a, Cell* p) {
  switch (op) {
    case O_Add: case O_Sub: case O_Mul: case O_Div:
                  return o_Math(a, p, op);
    case O_Vec:   return o_Vec(a, p);
    case O_Skip:  return o_Skip(a, p);
    case O_Take:  return o_Take(a, p);
    case O_Range: return o_Range(a, p);
    case O_Cycle: return o_Cycle(a, p);
    case O_Emit:  return o_Emit(a, p);
    case O_Map:   return o_Map(a, p);
    case O_Where: return o_Where(a, p);
    case O_Str:   return o_Str(a, p);
    case O_Print: case O_Priln:
                  return o_Print(a, p, op == O_Priln);
    case O_Val:   return o_Val(a, p);
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
    Cell *head, *arg;
    Value headVal = eval(a, p);
    head = arg = new Cell{headVal};
    //Handle short-circuited forms
    if (head->value.op() == O_If)
      return eval(cellAt(a, eval(a->next, p).tru() ? 2 : 3), p);
    //Continue collecting arguments
    while ((a = a->next)) {
      arg = arg->next ? arg->next : arg;
      arg->next = new Cell{eval(a, p)};
    }
    //... then call the operation/lambda/function
    auto ret = Value();
    if (head->value.type() == T_Op)
      ret = head->value.op()
        ? exeOp(head->value.op(), head->next, p)
        : head->value; //This can happen with REPL non-form evals
    else
    if (head->value.type() == T_Lamb) {
      Cell lHead = Cell{Value{head->value.data(), T_Cell}};
      ret = eval(&lHead, head->next);
      lHead.value.kill();
    } else
    if (head->value.type() == T_Func)
      ret = exeFunc(head->value.func(), head->next);
    delete head;
    return ret;
  }
  //Return parameter or nil
  if (t == T_Para)
    return p ? valAt(p, nullptr, a->value.u08()) : Value();
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
      Cell *args, *arg;
      args = arg = new Cell{liztAt(m->sources[0], at)};
      for (argnum v = 1, vLen = m->sources.size(); v < vLen; ++v) {
        arg = arg->next ? arg->next : arg;
        arg->next = new Cell{liztAt(m->sources[v], at)};
      }
      m->head->next = args;
      //Create a temporary form Cell
      Cell form = Cell{Value(Data{.cell=m->head}, T_Cell)};
      Value v = eval(&form);
      form.value.kill(); //Ensure head is not destroyed with form
      m->head->next = nullptr;
      delete args;
      return v;
    }
  }
  return Value();
}

//Returns remaining Lizt items as T_Vec
Value EVM::liztFrom (Lizt* l, veclen from) {
  if (l->isInf()) return Value();
  auto list = immer::vector<Value>();
  for (auto i = from; i < l->len; ++i)
    list = list.push_back(liztAt(l, i));
  return Value(Data{.ptr=new immer::vector<Value>(list)}, T_Vec);
}
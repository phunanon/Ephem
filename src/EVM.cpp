#include "EVM.hpp"
#include <cstdint>
#include <cstring>
#include <limits>

void EVM::addFunc (fid id, vector<Cell*> cells) {
  Cell* next = new Cell();
  for (auto c : cells) {
    Cell* link = new Cell();
    next->next = link;
    link->value = Value(Data{.cell=c}, T_Cell);
    next = link;
  }
  if (funcs.find(id) != funcs.end())
    delete funcs[id];
  funcs[id] = next;
}


Value EVM::exe (fid id, Cell* params) {
  auto cell = funcs[id];
  if (!cell) return Value();
  return val(cell, params);
}


//Returns an evaluated traversal across cell->next, or nil
Value EVM::nextValBy (Cell* a, Cell* p, argnum by) {
  if (by++)
    while (--by && a->next)
      a = a->next;
  else
    return val(a, p);
  return by ? Value() : val(a, p);
}


Value EVM::o_If (Cell* cond, Cell* p) {
  return nextValBy(cond, p, val(cond, p).tru() ? 1 : 2);
}


Value EVM::o_Math (Cell* a, Cell* p, Op op) {
  Value firstVal = val(a, p);
  a = a->next;
  const Type resultT = firstVal.type;
  const bool isFloat = resultT == T_D32;
  uint32_t iResult;
  memcpy(&iResult, &firstVal.data.u32, firstVal.size());
  float fResult = *(float*)(&iResult);
  while (a) {
    Value v = val(a, p);
    uint32_t iNext = 0;
    memcpy(&iNext, &v.data.u32, v.size());
    if (isFloat) {
      float fNext = v.type == T_D32 ? *(float*)(&iNext) : (float)iNext;
      switch (op) {
        case O_Add: fResult += fNext; break;
        case O_Sub: fResult -= fNext; break;
        case O_Mul: fResult *= fNext; break;
        case O_Div: fResult /= fNext; break;
      }
    } else {
      if (v.type == T_D32) iNext = *(float*)(&iNext);
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
    vect = vect.push_back(val(a, p));
    a = a->next;
  }
  auto vectPtr = new immer::vector<Value>(vect);
  return Value(Data{.ptr=vectPtr}, T_Vec);
}


//Returns a range Lizt.
//  e.g. (range) (range to) (range from to) (range from to step)
Value EVM::o_Range (Cell* a, Cell* p) {
  auto range = Lizt::Range();
  range.step = a ? 1 : 0;
  if (a && !a->next) {
    range.to = val(a, p).s32();
  }
  if (a && a->next) {
    range.next = val(a, p).s32();
    range.to = val(a->next, p).s32();
    if (a->next->next)
      range.step = val(a->next->next, p).s32();
  }
  return Value(Data{.ptr=Lizt::range(range)}, T_Lizt);
}


Value EVM::o_Cycle (Cell* a, Cell* p) {
  auto vals = vector<Value>();
  while (a) {
    vals.push_back(val(a, p));
    a = a->next;
  }
  return Value(Data{.ptr=Lizt::cycle(vals)}, T_Lizt);
}


Value EVM::o_Map (Cell* a, Cell* p) {
  Cell* head;
  //Ensure function is an op, lambda, (or function) TODO
  {
    Type t = a->value.type;
    if (t == T_Lamb || t == T_Op) head = new Cell{a->value};
    else {
      Value v = val(a, p);
      if (v.type != T_Lamb && v.type != T_Op)
        return Value();
      head = new Cell{v};
    }
  }
  auto vectors = vector<Lizt*>();
  while ((a = a->next))
    vectors.push_back(Lizt::list(val(a, p)));
  return Value(Data{.ptr=Lizt::map(head, vectors)}, T_Lizt);
}


Value EVM::o_Str (Cell* a, Cell* p) {
  auto str = new string();
  while (a) {
    *str += toStr(val(a, p));
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
  return val(a, p);
}


Value EVM::exe (Op op, Cell* a, Cell* p) {
  switch (op) {
    case O_If:    return o_If(a, p);
    case O_Add: case O_Sub: case O_Mul: case O_Div:
                  return o_Math(a, p, op);
    case O_Vec:   return o_Vec(a, p);
    case O_Cycle: return o_Cycle(a, p);
    case O_Range: return o_Range(a, p);
    case O_Map:   return o_Map(a, p);
    case O_Str:   return o_Str(a, p);
    case O_Print: case O_Priln:
                  return o_Print(a, p, op == O_Priln);
    case O_Val:   return o_Val(a, p);
  }
  return Value();
}

Value EVM::val (Cell* a, Cell* p) {
  Type t = a->value.type;
  if (t == T_Cell || t == T_Lamb) {
    //Have lambdas use cell's next as parameter list
    if (t == T_Lamb)
      p = a->next;
    //Evaluate for an operation
    Cell* head = a->value.cell();
    Value v = val(head, p);
    Op op = v.op();
    //Either execute an op as part of a form,
    //  or return the value if it wasn't an op.
    //    This can happen with REPL non-form evaluations.
    return op ? exe(op, head->next, p) : v;
  }
  //Return parameter or nil
  if (t == T_Para)
    return p ? nextValBy(p, nullptr, a->value.u08()) : Value();
  //TODO: variables
  return a->value;
}



string EVM::toStr (Value v) {
  switch (v.type) {
    case T_N:   return string("N");
    case T_U08: return to_string(v.u08());
    case T_S08: return string(1, v.s08());
    case T_U32: return to_string(v.u32());
    case T_S32: return to_string(v.s32());
    case T_D32: return to_string(v.d32());
    case T_Str: return v.str();
    case T_Vec: {
      auto vect = *vec(v);
      auto vLen = vect.size();
      if (!vLen) return "[]";
      string vecStr = toStr((Value)vect[0]);
      for (veclen i = 1; i < vLen; ++i)
        vecStr += " " + toStr((Value)vect[i]);
      return "["+ vecStr +"]";
    }
    case T_Lizt: return toStr(liztRest(v.lizt())); break;
    //TODO
  }
  return string("?");
}

//Returns next value of the lazy list
Value EVM::liztNext (Lizt* l) {
  if (l->isEmpty()) return Value();
  switch (l->type) {
    case LiztT::P_Vec: {
      auto state = (queue<Value>*)l->state;
      auto v = state->front();
      state->pop();
      return v;
    }
    case LiztT::P_Cycle: {
      auto c = (Lizt::Cycle*)l->state;
      auto v = c->items[c->i++];
      if (c->i == c->items.size())
        c->i = 0;
      return v;
    }
    case LiztT::P_Range: {
      auto r = (Lizt::Range*)l->state;
      int32_t next = r->next;
      r->next += r->step ? r->step : 1;
      return Value(Data{.s32=next}, T_S32);
    }
    case LiztT::P_Map: {
      auto m = (Lizt::Map*)l->state;
      Cell* args = nullptr;
      {
        Cell* arg = nullptr;
        //Take one item from each source vector and turn it into an argument
        for (argnum v = 0, vLen = m->sources.size(); v < vLen; ++v) {
          (arg ? arg->next : arg) = new Cell{liztNext(m->sources[v])};
          if (!args) args = arg;
        }
      }
      m->head->next = args;
      Cell form = Cell{Value(Data{.cell=m->head}, T_Cell)};
      Value v = val(&form);
      form.value.data.ptr = m->head->next = nullptr;
      delete args;
      return v;
    }
  }
  return Value();
}

//Returns remaining Lizt items as T_Vec
Value EVM::liztRest (Lizt* l) {
  if (l->isInfinite()) return Value();
  auto list = immer::vector<Value>();
  while (!l->isEmpty())
    list = list.push_back(liztNext(l));
  return Value(Data{.ptr=new immer::vector<Value>(list)}, T_Vec);
}
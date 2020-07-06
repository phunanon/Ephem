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
  Cell form = Cell{Value(Data{.cell=head}, T_Cell)};
  //Evaluate each argument for vectors
  veclen smallest = std::numeric_limits<veclen>::max();
  auto vectors = vector<immer::vector<Value>>();
  while ((a = a->next)) {
    Value v = val(a, p);
    if (v.type == T_Vec) {
      vectors.push_back(*vec(v));
      veclen len = vectors.back().size();
      if (len < smallest) smallest = len;
    }
  }
  auto newVec = immer::vector<Value>();
  //For each item
  for (veclen i = 0, vLen = vectors.size(); i < smallest; ++i) {
    Cell *args, *arg;
    args = arg = nullptr;
    //... in each vector
    for (argnum v = 0; v < vLen; ++v) {
      (arg ? arg->next : arg) = new Cell{vectors[v][i]};
      if (!args) args = arg;
    }
    head->next = args;
    newVec = newVec.push_back(val(&form, p));
    delete args;
  }
  head->next = nullptr;
  auto newVecPtr = new immer::vector<Value>(newVec);
  return Value(Data{.ptr=newVecPtr}, T_Vec);
}


Value EVM::o_Str (Cell* a, Cell* p) {
  auto str = new string();
  while (a) {
    *str += val(a, p).toStr();
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

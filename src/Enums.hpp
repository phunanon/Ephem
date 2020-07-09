#pragma once
#include <cstdint>
#include <cstring>

typedef size_t   fid;    //Func ID/hash
typedef uint8_t  argnum; //Parameter number
typedef int32_t  veclen; //Vec or Lizt len
typedef uint16_t refnum; //ARC reference number

const refnum NUM_OBJ = 20'000;

enum Type : uint8_t {
  T_N, T_Op, T_Cell, T_Lamb, T_Func, T_Para, T_Var,
  T_U08, T_S08, T_U16, T_S16, T_U32, T_S32, T_D32,
  T_Boo, T_Str, T_Vec, T_Lizt
};

enum Op : uint8_t {
  O_None, O_If, O_Add, O_Sub, O_Mul, O_Div, O_Mod,
  O_Alike, O_NAlike, O_Equal, O_NEqual,
  O_Vec, O_Skip, O_Take, O_Range, O_Cycle, O_Emit,
  O_Map, O_Where,
  O_Str, O_Print, O_Prinln, O_Val, O_Do
};

const char* const ops[] = {
  "none", "if", "+", "-", "*", "/", "mod",
  "=", "!=", "==", "!==",
  "vec", "skip", "take", "range", "cycle", "emit",
  "map", "where",
  "str", "print", "println", "val", "do", 0
};
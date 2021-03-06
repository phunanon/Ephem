#pragma once
#include <memory>
#include <string>
#include "Env.hpp"
#include "Cell.hpp"
using namespace std;

class FuncList {
  vector<fid> ids = vector<fid>();
  vector<vector<Cell*>> funcs = vector<vector<Cell*>>();
  fid _numFuncs;
public:
  ~FuncList ();
  fid numFuncs () { return _numFuncs; };
  int funcAt (fid);
  vector<Cell*>* get (fid);
  void remove (fid);
  void add (fid, vector<Cell*>);
};

class EVM {
public:
  EVM (Env e) { env = e; }

  void addFunc (fid, vector<Cell*>);
  void removeFunc (fid);
  Value exeFunc (fid, Cell* = nullptr);
  string toStr (Value);

private:
  Env env;
  FuncList funcs = FuncList();
  bool doRecur = false;
  Cell* recurArgs = nullptr;
  Cell* recurGarbage = nullptr;

  Value exeOp (Op, Cell*);
  Value eval (Cell*, Cell* = nullptr);
  Value valAt (Cell*, argnum);
  Cell* cellAt (Cell*, argnum);
  bool  areAlike (Value, Value);
  Value o_Math   (Cell*, Op);
  Value o_Equal  (Cell*, Op);
  Value o_Vec    (Cell*);
  Value o_Skip   (Cell*);
  Value o_Take   (Cell*);
  Value o_Range  (Cell*);
  Value o_Cycle  (Cell*);
  Value o_Emit   (Cell*);
  Value o_Map    (Cell*);
  Value o_Where  (Cell*);
  Value o_Str    (Cell*);
  Value o_Print  (Cell*, bool);
  Value liztAt   (Lizt*, veclen);
  Value liztFrom (Lizt*, veclen);
};
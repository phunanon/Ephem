#pragma once
#include <memory>
#include <string>
#include <map>
#include "Cell.hpp"
using namespace std;

class EVM {
public:
  void addFunc (fid, vector<Cell*>);
  void removeFunc (fid);
  Value exeFunc (fid, Cell* = nullptr);
  string toStr (Value);

private:
  map<fid, vector<Cell*>> funcs = map<fid, vector<Cell*>>();
  Value exeOp (Op, Cell*, Cell*);
  Value eval (Cell*, Cell* = nullptr);
  Value valAt (Cell*, argnum);
  Cell* cellAt (Cell*, argnum);
  Cell* makeHead (Cell*);
  Value o_Math  (Cell*, Op);
  Value o_Vec   (Cell*);
  Value o_Skip  (Cell*);
  Value o_Take  (Cell*);
  Value o_Range (Cell*);
  Value o_Cycle (Cell*);
  Value o_Emit  (Cell*);
  Value o_Map   (Cell*);
  Value o_Where (Cell*);
  Value o_Str   (Cell*);
  Value o_Print (Cell*, bool);
  Value o_Val   (Cell*);
  Value liztAt   (Lizt*, veclen);
  Value liztFrom (Lizt*, veclen);
};

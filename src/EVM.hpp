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
  Value valAt (Cell*, Cell*, argnum);
  Cell* cellAt (Cell*, argnum);
  Cell* makeHead (Cell*, Cell*);
  Value o_Math  (Cell*, Cell*, Op);
  Value o_Vec   (Cell*, Cell*);
  Value o_Skip  (Cell*, Cell*);
  Value o_Take  (Cell*, Cell*);
  Value o_Range (Cell*, Cell*);
  Value o_Cycle (Cell*, Cell*);
  Value o_Emit  (Cell*, Cell*);
  Value o_Map   (Cell*, Cell*);
  Value o_Where (Cell*, Cell*);
  Value o_Str   (Cell*, Cell*);
  Value o_Print (Cell*, Cell*, bool);
  Value o_Val   (Cell*, Cell*);
  Value liztAt   (Lizt*, veclen);
  Value liztFrom (Lizt*, veclen);
};

#pragma once
#include <memory>
#include <string>
#include <map>
#include "Cell.hpp"
using namespace std;

class EVM {
public:
  void addFunc (fid, vector<Cell*>);
  Value exe (fid, Cell* = nullptr);
  string toStr (Value);

private:
  map<fid, Cell*> funcs = map<fid, Cell*>();
  Value exe (Op, Cell*, Cell*);
  Value val (Cell*, Cell* = nullptr);
  Value nextValBy (Cell*, Cell*, argnum);
  Value o_If    (Cell*, Cell*);
  Value o_Math  (Cell*, Cell*, Op);
  Value o_Vec   (Cell*, Cell*);
  Value o_Range (Cell*, Cell*);
  Value o_Cycle (Cell*, Cell*);
  Value o_Map   (Cell*, Cell*);
  Value o_Str   (Cell*, Cell*);
  Value o_Print (Cell*, Cell*, bool);
  Value o_Val   (Cell*, Cell*);
  Value liztNext (Lizt*);
  Value liztRest (Lizt*);
};

#pragma once
#include <map>
#include <vector>
#include "Cell.hpp"

struct Parser {
  static map<fid, vector<Cell*>> parse (string source);
};
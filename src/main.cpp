#include <iostream>
#include <string>
#include <vector>
#include "linenoise/linenoise.h"
#include "Parser.hpp"
#include "EVM.hpp"
using namespace std;

int main () {
  printf("Ephem REPL. %% gives previous result. Arrow keys navigate history/entry. ^C to quit.\n");
  EVM vm = EVM();
  Cell* previous = nullptr;
  while (true) {
    string input;
    {
      char* line = linenoise("> ");
      if (!line) return 0;
      linenoiseHistoryAdd(line);
      input = string(line);
      if (!input.length()) continue;
      free(line);
    }
    auto parsed = Parser::parse(input);
    vm.removeFunc(0);
    bool hasEntry = false;
    for (auto func : parsed) {
      if (func.first == 0) hasEntry = true;
      vm.addFunc(func.first, func.second);
    }
    if (hasEntry) {
      Cell* evaled = new Cell{vm.exeFunc(0, previous)};
      delete previous;
      previous = evaled;
      printf("%s\n", vm.toStr(evaled->value).c_str());
    }
  }
}

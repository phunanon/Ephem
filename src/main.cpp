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
    for (auto func : parsed)
      vm.addFunc(func.first, func.second);
    {
      Cell* evaled = new Cell{vm.exe(0, previous)};
      delete previous;
      previous = evaled;
    }
    printf("%s\n", vm.toStr(previous->value).c_str());
  }
}

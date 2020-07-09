#include <string>
#include <vector>
#include "linenoise/linenoise.h"
#include "Parser.hpp"
#include "EVM.hpp"
using namespace std;

void vmSession () {
  printf("Ephem REPL. %% gives previous result. Arrow keys navigate history/entry. q or ^C to quit.\n");
  EVM vm = EVM();
  Cell* previous = nullptr;
  while (true) {
    string input;
    {
      char* line = linenoise("> ");
      if (!line) return;
      linenoiseHistoryAdd(line);
      input = string(line);
      if (!input.length()) continue;
      free(line);
      if (input == "q") break;
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
  delete previous;
}

int main () {
  vmSession();
  if (Cell::checkMemLeak())
    printf("Warning: ARC memory leak detected.\n");
  return 0;
}

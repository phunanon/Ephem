#include <mimalloc.h>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include "linenoise/linenoise.h"
#include "keypresses.c"
#include "Parser.hpp"
#include "EVM.hpp"
using namespace std;

bool parseAndLoad (EVM &vm, string input) {
  bool hasEntry = false;
  for (auto func : Parser::parse(input)) {
    hasEntry |= !func.first;
    vm.addFunc(func.first, func.second);
  }
  return hasEntry;
}

void repl () {
  printf("Ephem REPL. %% gives previous result. Arrow keys navigate history/entry. q or ^C to quit.\n");
  EVM vm = EVM(Env());
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
    vm.removeFunc(0);
    if (parseAndLoad(vm, input)) {
      Cell* evaled = new Cell{vm.exeFunc(0, previous)};
      delete previous;
      previous = evaled;
      printf("%s\n", vm.toStr(evaled->val).c_str());
    }
  }
  delete previous;
}

int main (int argc, char *argv[]) {
  kb_listen();
  if (argc > 1) {
    ifstream infile{string(argv[1])};
    EVM vm = EVM(Env());
    parseAndLoad(vm, {istreambuf_iterator<char>(infile), istreambuf_iterator<char>()});
    vm.exeFunc(0, nullptr);
  } else repl();

  if (Cell::checkMemLeak())
    printf("Warning: ARC memory leak detected.\n");
  
  return 0;
}
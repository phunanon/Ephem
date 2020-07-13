#include "Env.hpp"
#include <cstdio>
#include <chrono>
#include <thread>
#include "keypresses.c"
#include "linenoise/linenoise.h"

void Env::print (const char* str) {
  printf("%s", str);
}

char Env::getKey () {
  return kb_has_key() ? getchar() : 0;
}

string* Env::getString (string prompt) {
  char* line = linenoise(prompt.c_str());
  if (!line) return new string();
  auto input = new string(line);
  free(line);
  return input;
}

void Env::sleep (uint ms) {
  this_thread::sleep_for(chrono::milliseconds(ms));
}
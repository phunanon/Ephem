#pragma once
#include <string>
using namespace std;

struct Env {
  static void    print (const char*);
  static char    getKey ();
  static string* getString (string);
  static void    sleep  (uint);
};
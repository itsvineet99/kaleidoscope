#include "codegen.h"
#include "parser.h"

int main() {
  InitializeParser();
  InitializeModule();
  MainLoop();
  DumpModule();

  return 0;
}

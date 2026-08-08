#include "codegen.h"
#include "parser.h"

int main() {
  InitializeCodegen();
  InitializeParser();
  MainLoop();

  return 0;
}

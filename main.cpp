#include "lexer.h"
#include <iostream>

int main() {
  std::cout << "spit > ";
  int token = getToken();
  while (token != 67) {
      std::cout << token << "\n";
      token = getToken();
  }
}

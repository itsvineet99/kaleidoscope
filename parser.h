#pragma once

#include <map>

// BinopPrecedence - This holds the precedence for each binary operator that is defined.
inline std::map<char, int> BinopPrecedence;

void InitializeParser();
void MainLoop();

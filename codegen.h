#pragma once

#include <memory>

class PrototypeAST;

void InitializeCodegen();
void AddCurrentModuleToJIT();
void AddExternPrototype(std::unique_ptr<PrototypeAST> Proto);
void ExecuteTopLevelExpression();

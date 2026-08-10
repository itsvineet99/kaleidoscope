#include "parser.h"
#include "AST.h"
#include "codegen.h"
#include "lexer.h"

#include "llvm/Support/raw_ostream.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


//===---------------------------------------------===////===---------===//
// Token Buffers 
//===---------------------------------------------===//

// CurTok is the current token the parser is looking at.
// getNextToken reads another token from the
// lexer and updates CurTok with its results.
static int CurToken;
static int getNextToken() { return CurToken = getToken(); }

//===---------------------------------------------===//
// Token Precedence
//===---------------------------------------------===//

// BinopPrecedence - This holds the precedence for each binary operator that is defined.
static std::map<char, int> BinopPrecedence;

// Get the precedence of the pending binary operator token.
static int GetTokenPrecedence() {
  if (!isascii(CurToken)) 
    return -1;

  int TokenPrec = BinopPrecedence[CurToken];
  if (TokenPrec <= 0)
    return -1;
  return TokenPrec;
}

//===---------------------------------------------===//
// Error Loggers
//===---------------------------------------------===//

// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

//===---------------------------------------------===//
// Parsing Fucntions (soul of this program?)
//===---------------------------------------------===//

//===---------------------------------------------===//
// parsing expresssoin
//===---------------------------------------------===//

static std::unique_ptr<ExprAST> ParseExpression();

//===---------------------------------------------===//
// parsing control flow statements 
//===---------------------------------------------===//

static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken(); // eat 'if'

  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;
  
  if (CurToken != tok_then)
    return LogError("expected then");
  getNextToken();

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (CurToken != tok_else)
    return LogError("expected else");

  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                     std::move(Else));
}

static std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();

  if (CurToken != tok_identifier)
    return LogError("expected identifier after for");

  std::string IdName = IdentifierStr;
  getNextToken();

  if (CurToken != '=')
    return LogError("expected '=' after for");
  getNextToken();

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurToken != ',')
    return LogError("expected ',' after for start value");
  getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  std::unique_ptr<ExprAST> Step;
  if (CurToken == ',') {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (CurToken != tok_in)
    return LogError("expected 'in' after for");
  getNextToken();

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken();
  return std::move(result);
}

// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.)
  auto V = ParseExpression();
  if (!V) 
    return nullptr;

  if (CurToken != ')') 
    return LogError("expected ')'");
  getNextToken();  // eat ).
  return V;
}

// identifierexpr
//   ::= identifier
//   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken(); // Eat identifer.

  if (CurToken != '(') 
    return std::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken(); // Eat (.
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurToken != ')') {
    while (true) {
      if (auto args = ParseExpression()) 
        Args.push_back(std::move(args));
      else 
        return nullptr;
      
      if (CurToken == ')')
        break;
      
      if (CurToken != ',') 
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }

  getNextToken(); // eat ).

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

// primary
//   ::= identifierexpr
//   ::= numberexpr
//   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurToken) {
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  case tok_if:
    return ParseIfExpr(); 
  case tok_for:
    return  ParseForExpr();
  
  default:
    return LogError("unknown token when expecting an expression");
  }
}

// binoprhs
//   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokenPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurToken;
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokenPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

// expression
//   ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;
  return ParseBinOpRHS(0, std::move(LHS));
}

//===---------------------------------------------===//
// parsing functions 
//===---------------------------------------------===//

// prototype
//   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (CurToken != tok_identifier)
    return LogErrorP("Expected function name in prototype");
  
  std::string FnName = IdentifierStr;
  getNextToken(); // eat fun name,

  if (CurToken != '(')
    return LogErrorP("Expected '(' in prototype");
  
  std::vector<std::string> ArgsNames;
  while (getNextToken() == tok_identifier)
    ArgsNames.push_back(IdentifierStr);
  if (CurToken != ')')
    return LogErrorP("Expected ')' in prototype");
  
  getNextToken(); // eat ).
  
  return std::make_unique<PrototypeAST>(FnName, ArgsNames);
}

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken(); // eat 'def'
  auto proto = ParsePrototype();
  if (!proto)
    return nullptr;
  
  if (auto E = ParseExpression()) 
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  return nullptr;
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto proto = std::make_unique<PrototypeAST>("__anon_expr",std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  }
  return nullptr;
}

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken(); // eat extern.
  return ParsePrototype();
}

//===---------------------------------------------===//
// Top Level Parsing 
//===---------------------------------------------===//

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(llvm::errs());
      fprintf(stderr, "\n");
      AddCurrentModuleToJIT();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(llvm::errs());
      fprintf(stderr, "\n");
      AddExternPrototype(std::move(ProtoAST));
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    if (FnAST->codegen())
      ExecuteTopLevelExpression();
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

//===---------------------------------------------===//
// Main Loop / Entry point
//===---------------------------------------------===//

// top ::= definition | external | expression | ';'

void InitializeParser() {
  // Install standard binary operators. 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;

  fprintf(stderr, "ready> ");
  getNextToken();
}

void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");

    switch (CurToken) {
      case tok_eof:
        return;
      case ';': 
        getNextToken();
        break;
      case tok_def:
        HandleDefinition();
        break;
      case tok_extern:
        HandleExtern();
        break;
      default:
        HandleTopLevelExpression();
        break;
    }
  }
}

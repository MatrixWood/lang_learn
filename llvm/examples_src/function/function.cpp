#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

int main(int argc, char** argv) {
  LLVMContext context;

  Module* module = new Module("FunctionModule", context);

  Type* voidType = Type::getVoidTy(context);
  FunctionType* functionType = FunctionType::get(voidType, false);
  Function* function = Function::Create(functionType, GlobalValue::ExternalLinkage, "HelloFunction", module);

  // Print the IR
  verifyFunction(*function);
  module->print(outs(), nullptr);

  return 0;
}
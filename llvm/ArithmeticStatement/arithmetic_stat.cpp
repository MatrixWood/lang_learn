#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include <vector>

using namespace llvm;

int main(int argc, char** argv) {
  LLVMContext context;
  IRBuilder<> builder(context);

  Module* module = new Module("Module", context);
  module->getOrInsertGlobal("GlobalVariable", builder.getInt32Ty());
  GlobalVariable* global_variable = module->getNamedGlobal("GlobalVariable");
  global_variable->setLinkage(GlobalValue::CommonLinkage);
  global_variable->setAlignment(MaybeAlign(4));

  std::vector<Type*> parameters(2, builder.getInt32Ty());
  FunctionType* functionType = FunctionType::get(builder.getInt32Ty(), parameters, false);
  Function* function = Function::Create(functionType, GlobalValue::ExternalLinkage, "Function", module);

  function->getArg(0)->setName("a");
  function->getArg(0)->setName("b");

  BasicBlock* block = BasicBlock::Create(context, "entry", function);
  builder.SetInsertPoint(block);

  Value* arg1 = function->getArg(0);
  ConstantInt* three = builder.getInt32(3);
  Value* result = builder.CreateMul(arg1, three, "multiplyResult");

  builder.CreateRet(result);
  verifyFunction(*function);
  module->print(outs(), nullptr);

  return 0;
}
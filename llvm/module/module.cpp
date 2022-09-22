#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main(int argc, char* argv[])
{
    LLVMContext context;
    Module* module = new Module("HelloModule", context);

    module->print(llvm::outs(), nullptr);

    return 0;
}
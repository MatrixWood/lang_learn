#include "lexer.h"
#include "version.h"

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace sl;

static llvm::cl::opt<std::string> input(llvm::cl::Positional, llvm::cl::desc("<input expression>"), llvm::cl::init(""));

int main(int argc, const char** argv) {
  llvm::InitLLVM llvmInitializer(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "simplelexer - a simple lexical analyzer\n");
  llvm::outs() << "Input: \"" << input << "\"\n";

  Lexer lex(input);

  Token token;
  lex.GetNext(token);
  while (token.GetType() != Token::kEOL) {
    llvm::outs() << "token type: " << token.GetType() << ", token text: \"" << token.GetText() << "\"\n";

    lex.GetNext(token);
  }

  llvm::outs() << "Hello World! (version " << sl::getSimpleLangVersion() << ")\n";
  return 0;
}
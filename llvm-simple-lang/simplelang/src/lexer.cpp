#include "lexer.h"

#include <cctype>

namespace sl {

void Lexer::InitToken(Token& token, const char* token_end, Token::TokenType type) {
  token.SetType(type);
  token.SetText(llvm::StringRef());
  buf_ptr_ = token_end;
}

void Lexer::GetNext(Token& token) {
  while (*buf_ptr_ && std::isspace(*buf_ptr_)) {
    ++buf_ptr_;
  }
  if (!*buf_ptr_) {
    token.SetType(Token::kEOL);
    return ;
  }
  if (std::isalpha(*buf_ptr_)) {
    const char* end = buf_ptr_ + 1;
    while (std::isalpha(*end)) { ++end; }
    llvm::StringRef name(buf_ptr_, end - buf_ptr_);
    Token::TokenType type = (name == "with" ? Token::kKeywordWith : Token::kIdent);
    InitToken(token, end, type);
    return ;
  }
  else if (std::isdigit(*buf_ptr_)) {
    const char* end = buf_ptr_ + 1;
    while (std::isdigit(*end)) { ++end; }
    InitToken(token, end, Token::kNumber);
    return ;
  }
  else {
    switch (*buf_ptr_) {
      case '+':
        InitToken(token, buf_ptr_ + 1, Token::kPlus);
        break;
      case '-':
        InitToken(token, buf_ptr_ + 1, Token::kMinus);
        break;
      case '*':
        InitToken(token, buf_ptr_ + 1, Token::kStar);
        break;
      case '/':
        InitToken(token, buf_ptr_ + 1, Token::kSlash);
        break;
      case '(':
        InitToken(token, buf_ptr_ + 1, Token::kLeftParen);
        break;
      case ')':
        InitToken(token, buf_ptr_ + 1, Token::kRightParen);
        break;
      case ':':
        InitToken(token, buf_ptr_ + 1, Token::kColon);
        break;
      case ',':
        InitToken(token, buf_ptr_ + 1, Token::kComma);
        break;
      default:
        InitToken(token, buf_ptr_ + 1, Token::kUnknown);
    }
    return ;
  }
}

}
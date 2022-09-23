#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"

namespace sl {

class Token {
public:
  enum TokenType {
    kEOL,
    kUnknown,
    kIdent,
    kNumber,
    kComma,
    kColon,
    kPlus,
    kMinus,
    kStar,
    kSlash,
    kLeftParen,
    kRightParen,
    kKeywordWith
  };

public:
  TokenType GetType() const {
    return type_;
  }

  void SetType(TokenType in_type) {
    type_ = in_type;
  }

  llvm::StringRef GetText() const {
    return text_;
  }

  void SetText(llvm::StringRef in_text) {
    text_ = in_text;
  }

  bool Is(TokenType in_type) const {
    return type_ == in_type;
  }

  bool IsOneOf(TokenType type1, TokenType type2) const {
    return Is(type1) || Is(type2);
  }

  template <typename... Ts>
  bool IsOneOf(TokenType type1, TokenType type2, Ts... types) const {
    return Is(type1) || IsOneOf(type2, types...);
  }

private:
  TokenType type_;
  llvm::StringRef text_;
};

class Lexer {
public:
  Lexer(const llvm::StringRef& buffer) {
    buf_start_ = buffer.begin();
    buf_ptr_ = buf_start_;
  }

  void GetNext(Token& token);

private:
  void InitToken(Token& token, const char* token_end, Token::TokenType type);

private:
  const char* buf_start_;
  const char* buf_ptr_;
};

}
#ifndef SOLVE_LEX_H_INCLUDED
#define SOLVE_LEX_H_INCLUDED

#include <iosfwd>
#include <string>
#include <string.h>

class token_test;

namespace lex {

enum class token_type {
    identifier,
    literal,
    op,

    eof
};
std::ostream& operator<<(std::ostream& os, token_type t);

class tokenizer;
class token {
public:
    token_type  type() const { return type_; }
    std::string str() const { return std::string(text_, text_ + length_); }
    size_t      length() const { return length_; }
private:
    friend tokenizer;
    friend token_test;

    explicit token();
    token(token_type type, const char* text, size_t length);

    token_type  type_;
    const char* text_;
    size_t      length_;

};

bool operator==(const token& a, const token& b);
bool operator!=(const token& a, const token& b);
std::ostream& operator<<(std::ostream& os, const token& t);

class tokenizer {
public:
    tokenizer(const char* text, size_t length) : text_(text), length_(length), pos_(0) {
        next_token();
    }

    const token& current() const {
        return current_;
    }

    token consume() {
        auto cur = current_;
        next_token();
        return cur;
    }

private:
    const char* text_;
    size_t      length_;
    size_t      pos_;
    token       current_;

    // initialize current_ with appropriate token type from pos_ bytes into text_
    // advance pos_ by the tokens length
    void next_token();
};


} // namespace lex

#endif

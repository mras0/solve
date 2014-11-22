#ifndef SOLVE_LEX_H_INCLUDED
#define SOLVE_LEX_H_INCLUDED

#include <iosfwd>
#include <string>
#include "source.h"

namespace lex {

enum class token_type {
    op_mul = '*',
    op_add = '+',
    op_sub = '-',
    op_div = '/',
    op_eq  = '=',

    identifier = 256,
    literal,
    separator,
    eof
};
std::ostream& operator<<(std::ostream& os, token_type t);

class tokenizer;
class token {
public:
    token_type       type() const { return type_; }
    std::string      str() const { return std::string{position_.data(), length_}; }
    source::position position() const { return position_; }
    size_t           length() const { return length_; }
private:
    friend tokenizer;

    token(token_type type, const source::position& pos, size_t length);

    token_type       type_;
    source::position position_;
    size_t           length_;
};

bool operator==(const token& a, const token& b);
bool operator!=(const token& a, const token& b);
std::ostream& operator<<(std::ostream& os, const token& t);

class tokenizer {
public:
    tokenizer(const source::file& source) : source_(source), position_(source), current_(token_type::eof, source::position{source}, 0) {
        next_token();
    }

    bool eof() const {
        return current().type() == token_type::eof;
    }

    token current() const {
        return current_;
    }

    token consume() {
        auto cur = current_;
        next_token();
        return cur;
    }

private:
    const source::file& source_;
    source::position    position_;
    token               current_;

    // initialize current_ with appropriate token type from pos_ bytes into text_
    // advance pos_ by the tokens length
    void next_token();
};


} // namespace lex

#endif

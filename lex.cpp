#include "lex.h"

#include <sstream>
#include <memory>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

namespace {

size_t try_parse(lex::token_type t, const char* text, size_t length) {
    assert(length != 0);
    if (t == lex::token_type::identifier) {
        size_t l = 0;
        while (l < length && isalpha(text[l])) {
            ++l;
        }
        return l;
    } else if (t == lex::token_type::literal) {
        std::istringstream iss{std::string{text, text+length}};
        assert(iss.tellg() == 0);
        double value;
        if (iss >> value) {
            const auto l = iss.eof() ? length : static_cast<size_t>(iss.tellg());
            assert(l != 0);
            assert(l <= length);
            return l;
        }
    } else if (t == lex::token_type::op) {
        for (const auto& op : "*+-/=") {
            if (*text == op) {
                return 1;
            }
        }
    } else {
        std::ostringstream oss;
        oss << "Unhandled token type '" << t << "' str='" << std::string{text, text+length} << "' in " << __func__;
        throw std::logic_error(oss.str());
    }
    return 0;
}

} // anonymous namespace

namespace lex {

std::ostream& operator<<(std::ostream& os, token_type t) {
    switch (t) {
#define HANDLE_TOKEN_TYPE(t) case token_type::t: os << #t; break
    HANDLE_TOKEN_TYPE(identifier);
    HANDLE_TOKEN_TYPE(literal);
    HANDLE_TOKEN_TYPE(op);
    HANDLE_TOKEN_TYPE(eof);
#undef HANDLE_TOKEN_TYPE
    }
    return os;
}

token::token() : type_(token_type::eof), text_(""), length_() {
}

token::token(token_type type, const char* text, size_t length) : type_(type), text_(text), length_(length) {
    assert(type != token_type::eof);
    assert(try_parse(type, text, length) == length);
}

bool operator==(const token& a, const token& b) {
    return a.type() == b.type() && a.str() == b.str();
}

bool operator!=(const token& a, const token& b) {
    return !(a == b);
}


std::ostream& operator<<(std::ostream& os, const token& t) {
    os << "{" << t.type() << " '" << t.str() << "'}";
    return os;
}

void tokenizer::next_token() {
    // loop until we reach eof or get a now whitespace character
    for (;;) {
        // handle end of text
        if (pos_ >= length_) {
            assert(pos_ == length_);
            current_ = token{}; // EOF
            return;
        }

        // we can now safely extract text_[pos_]
        const char* const text_pos = text_ + pos_;
        const size_t      remaining = length_ - pos_;

        // skip whitespace
        if (isspace(*text_pos)) {
            pos_++;
            continue;
        }

        for (const auto& t : { token_type::identifier, token_type::literal, token_type::op}) {
            if (const auto l = try_parse(t, text_pos, remaining)) {
                assert(l <= remaining);
                pos_ += l;
                current_ = token{t, text_pos, l};
                return;
            }
        }

        throw std::runtime_error("Parse error at: " + std::string(text_pos, text_pos+remaining));
    }
}

} // namespace lex

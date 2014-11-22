#include "lex.h"

#include <sstream>
#include <stdexcept>
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
    HANDLE_TOKEN_TYPE(separator);
    HANDLE_TOKEN_TYPE(eof);
#undef HANDLE_TOKEN_TYPE
    }
    return os;
}

token::token(token_type type, const source::position& position, size_t length) : type_(type), position_(position), length_(length) {
    assert((type == token_type::eof && length_ == 0)
            || (type == token_type::separator && length_ == 1)
            || try_parse(type, position.data(), length_) == length_);
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
        if (position_.index() >= source_.length()) {
            assert(position_.index() == source_.length());
            current_ = token{token_type::eof, position_, 0};
            return;
        }

        // we can now safely extract text_[position_.index()]
        const char* const text = source_.data() + position_.index();
        const size_t      remaining = source_.length() - position_.index();

        // handle newline specially
        if (*text == '\n') {
            current_ = token{token_type::separator, position_, 1};
            position_ = position_.advanced_ws(*text);
            return;
        }

        // skip whitespace
        if (isspace(*text)) {
            position_ = position_.advanced_ws(*text);
            continue;
        }

        for (const auto& t : { token_type::op, token_type::identifier, token_type::literal }) {
            if (const auto l = try_parse(t, text, remaining)) {
                assert(l <= remaining);
                current_ = token{t, position_, l};
                position_ = position_.advanced_n(l);
                return;
            }
        }

        throw std::runtime_error("Parse error at: " + std::string(text, text+remaining));
    }
}

} // namespace lex

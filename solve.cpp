#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include <initializer_list>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

enum class token_type {
    identifier,
    literal,
    op,

    eof
};

std::ostream& operator<<(std::ostream& os,  token_type t) {
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


static size_t try_parse(token_type t, const char* text, size_t length) {
    assert(length != 0);
    if (t == token_type::identifier) {
        size_t l = 0;
        while (l < length && isalpha(text[l])) {
            ++l;
        }
        return l;
    } else if (t == token_type::literal) {
        std::istringstream iss{std::string{text, text+length}};
        assert(iss.tellg() == 0);
        double value;
        if (iss >> value) {
            const auto l = iss.eof() ? length : static_cast<size_t>(iss.tellg());
            assert(l != 0);
            assert(l <= length);
            return l;
        }
    } else if (t == token_type::op) {
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


class token {
public:
    static const size_t nts = size_t(-1);
    static token eof() { return token{token_type::eof, "", 0}; }
    static token identifier(const char* str, size_t len = nts) {
        return token{token_type::identifier, str, len == nts ? strlen(str) : len};
    }
    static token literal(const char* str, size_t len = nts) {
        return token{token_type::literal, str, len == nts ? strlen(str) : len};
    }
    static token op(const char* str, size_t len = nts) {
        return token{token_type::op, str, len == nts ? strlen(str) : len};
    }

    token_type  type() const { return type_; }
    std::string str() const { return std::string(text_, text_ + length_); }
    size_t      length() const { return length_; }
private:
    friend class tokenizer;

    token(token_type type, const char* text, size_t length) : type_(type), text_(text), length_(length) {
        assert((type == token_type::eof && length == 0) || try_parse(type, text, length) == length);
    }

    token_type  type_;
    const char* text_;
    size_t      length_;

};

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

class tokenizer {
public:
    tokenizer(const char* text, size_t length) : text_(text), length_(length), pos_(0), current_(token::eof()) {
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
    void next_token() {
        // loop until we reach eof or get a now whitespace character
        for (;;) {
            // handle end of text
            if (pos_ >= length_) {
                assert(pos_ == length_);
                current_ = token::eof();
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
};

void test_tokenizer(const std::string& text, std::initializer_list<token> expected_tokens)
{
    tokenizer t{text.c_str(), text.length()};
    for (const auto& expected_token : expected_tokens) {
        auto current_token = t.consume();
        std::cout << current_token << " " << std::flush;
        if (current_token != expected_token) {
            std::cerr << "Tokenizer error. Expeceted " << expected_token << " got " << current_token << std::endl;
            abort();
        }
    }
    assert(t.current().type() == token_type::eof);
    std::cout << "OK!\n";
}

int main()
{
    test_tokenizer("",{token::eof()});
    test_tokenizer("\t\n\r   ",{token::eof()});
    test_tokenizer(" id  ",{token::identifier("id"), token::eof()});
    test_tokenizer("3.14",{token::literal("3.14"), token::eof()});
    test_tokenizer("=\n",{token::op("="), token::eof()});
    test_tokenizer("\thello 42",{token::identifier("hello"), token::literal("42"), token::eof()});
    test_tokenizer("\tx + 1e3 = 20",{token::identifier("x"), token::op("+"), token::literal("1e3"), token::op("="), token::literal("20"), token::eof()});

    const char* const program = R"(
        vals       = 2000
        valsize    = 8
        freq       = 1
        bspersec   = vals * valsize * freq
        bsperday   = bspersec * 60 * 60 * 24
    )";
    test_tokenizer(program, {
        token::identifier("vals"),     token::op("="), token::literal("2000"),
        token::identifier("valsize"),  token::op("="), token::literal("8"),
        token::identifier("freq"),     token::op("="), token::literal("1"),
        token::identifier("bspersec"), token::op("="), token::identifier("vals"), token::op("*"), token::identifier("valsize"), token::op("*"), token::identifier("freq"),
        token::identifier("bsperday"), token::op("="), token::identifier("bspersec"), token::op("*"), token::literal("60"), token::op("*"), token::literal("60"), token::op("*"), token::literal("24"),
        token::eof(),
    });
}

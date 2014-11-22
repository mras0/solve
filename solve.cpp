#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include <initializer_list>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

namespace ast {

class node {
public:
    virtual ~node() {};
};

class const_node : public node {
public:
    explicit const_node(double value) : value_(value) {
    }

private:
    double value_;
};

} // namespace ast

enum class token_type {
    constant,
    identifier,

    eof
};

std::ostream& operator<<(std::ostream& os,  token_type t) {
    switch (t) {
#define HANDLE_TOKEN_TYPE(t) case token_type::t: os << #t; break
    HANDLE_TOKEN_TYPE(constant);
    HANDLE_TOKEN_TYPE(identifier);
    HANDLE_TOKEN_TYPE(eof);
#undef HANDLE_TOKEN_TYPE
    }
    return os;
}


class token {
public:
    virtual ~token() {}

    token_type  type() const { return type_; }
    std::string str() const { return std::string(text_, text_ + length_); }
    size_t      length() const { return length_; }
protected:
    token(token_type type, const char* text, size_t length) : type_(type), text_(text), length_(length) {
    }

private:
    token_type  type_;
    const char* text_;
    size_t      length_;
};
typedef std::unique_ptr<token> token_ptr;

bool operator==(const token& a, const token& b) {
    return a.type() == b.type() && a.str() == b.str();
}

bool operator!=(const token& a, const token& b) {
    return !(a == b);
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    os << "<" << t.type() << " '" << t.str() << "'>";
    return os;
}

class constant_token : public token {
public:
    static token_ptr try_parse(const char* text, size_t length) {
        std::istringstream iss{std::string{text, text+length}};
        assert(iss.tellg() == 0);
        double value;
        if (iss >> value) {
            const auto l = iss.eof() ? length : static_cast<size_t>(iss.tellg());
            assert(l != 0);
            assert(l <= length);
            return token_ptr{new constant_token{text, l, value}};
        }
        return nullptr;
    }

    double value() const { return value_; }
private:
    double value_;

    constant_token(const char* text, size_t length, double value) : token(token_type::constant, text, length), value_(value) {
    }
};

class identifier_token : public token {
public:
    static token_ptr try_parse(const char* text, size_t length) {
        size_t l = 0;
        while (l < length && isalpha(text[l])) {
            ++l;
        }
        return l > 0 ? token_ptr{new identifier_token{text, l}} : nullptr;
    }
private:
    identifier_token(const char* text, size_t length) : token(token_type::identifier, text, length) {
    }
};

class eof_token : public token {
public:
    explicit eof_token() : token(token_type::eof, "", 0) {
    }
};

token_ptr make_token(token_type type, const std::string& str)
{
    token_ptr p;
    if (type == token_type::constant) {
        p = constant_token::try_parse(str.c_str(), str.length());
    } else if (type == token_type::identifier) {
        p = identifier_token::try_parse(str.c_str(), str.length());
    } else {
        std::ostringstream oss;
        oss << "Unhandled token type '" << type << "' str='" << str << "'";
        throw std::logic_error(oss.str());
    }

    if (!p) {
        std::ostringstream oss;
        oss << "Unable to make token type '" << type << "' out of '" << str << "'";
        throw std::logic_error(oss.str());
    }
    return p;
}
token_ptr make_token(token_type type)
{
    assert(type == token_type::eof);
    return token_ptr{new eof_token{}};
}

class tokenizer {
public:
    tokenizer(const char* text, size_t length) : text_(text), length_(length), pos_(0) {
        next_token();
    }

    const token& peek() const {
        assert(current_);
        return *current_;
    }

    token_ptr consume() {
        auto cur = std::move(current_);
        next_token();
        return cur;
    }

private:
    const char* text_;
    size_t      length_;
    size_t      pos_;
    token_ptr   current_;

    // initialize current_ with appropriate token type from pos_ bytes into text_
    // advance pos_ by the tokens length
    void next_token() {
        // loop until we reach eof or get a now whitespace character
        for (;;) {
            // handle end of text
            if (pos_ >= length_) {
                assert(pos_ == length_);
                current_ = make_token(token_type::eof);
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

            static const decltype(&identifier_token::try_parse) parsers[] = {
                &identifier_token::try_parse,
                &constant_token::try_parse
            };

            for (const auto& p : parsers) {
                if (current_ = p(text_pos, remaining)) {
                    assert(current_->length() <= remaining);
                    pos_ += current_->length();
                    return;
                }
            }

            throw std::runtime_error("Parse error at: " + std::string(text_pos, text_pos+remaining));
        }
    }
};

void test_tokenizer(const std::string& text, std::initializer_list<token_ptr> expected_tokens)
{
    tokenizer t{text.c_str(), text.length()};
    for (const auto& expected_token : expected_tokens) {
        auto current_token = t.consume();
        std::cout << *current_token << " ";
        if (*current_token != *expected_token) {
            std::cerr << "Tokenizer error. Expeceted " << *expected_token << " got " << *current_token << std::endl;
            abort();
        }
    }
    assert(t.peek().type() == token_type::eof);
    std::cout << "OK!\n";
}

int main()
{
    test_tokenizer("",{make_token(token_type::eof)});
    test_tokenizer("\t\n\r   ",{make_token(token_type::eof)});
    test_tokenizer(" id  ",{make_token(token_type::identifier, "id"), make_token(token_type::eof)});
    test_tokenizer("3.14",{make_token(token_type::constant, "3.14"), make_token(token_type::eof)});
    test_tokenizer("\thello 42",{make_token(token_type::identifier, "hello"), make_token(token_type::constant, "42"), make_token(token_type::eof)});

#if 0
    const char* const program = R"(
        vals       = 2000
        valsize    = 8
        freq       = 1
        bspersec   = vals * valsize * freq
        bsperday   = bspersec * 60 * 60 * 24
    )";
#endif
}

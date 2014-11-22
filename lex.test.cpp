#include "lex.h"
#include <iostream>
#include <assert.h>

namespace {

struct expected_token {
    expected_token(lex::token_type t, const char* str, size_t line, size_t col) : type(t), str(str), line(line), col(col) {
    }
    lex::token_type type;
    std::string     str;
    size_t          line;
    size_t          col;

    void verify(const lex::token& actual) const {
        if (actual.type() != type || actual.str() != str) {
            std::cerr << "Tokenizer error. Expeceted " << type << " got " << actual << " at " << actual.position() << std::endl;
            assert(false);
        }
        if ((line && actual.position().line() != line) || (col && actual.position().col() != col)) {
            std::cerr << "Tokenizer error. Expeceted line " << line << " col " << col << " got " << actual << " at " << actual.position() << std::endl;
            assert(false);
        }
     }
};

expected_token sep(size_t line=0, size_t col=0) {
    return expected_token{lex::token_type::separator, "\n", line, col};
}

expected_token eof(size_t line=0, size_t col=0) {
    return expected_token{lex::token_type::eof, "", line, col};
}

expected_token identifier(const char* str, size_t line=0, size_t col=0) {
    return expected_token{lex::token_type::identifier, str, line, col};
}

expected_token literal(const char* str, size_t line=0, size_t col=0) {
    return expected_token{lex::token_type::literal, str, line, col};
}

expected_token op(const char* str, size_t line=0, size_t col=0) {
    assert(str[1] == 0);
    return expected_token{static_cast<lex::token_type>(str[0]), str, line, col};
}

void test_tokenizer(const std::string& text, std::initializer_list<expected_token> expected_tokens) {
    source::file src{text, text};
    lex::tokenizer t{src};
    for (const auto& expected_token : expected_tokens) {
        auto current_token = t.consume();
        //std::cout << current_token << " " << std::flush;
        expected_token.verify(current_token);
    }
    assert(t.current().type() == lex::token_type::eof);
}

} // unnamed namespace

void lex_test() {
    test_tokenizer("", { eof(1,1) });
    test_tokenizer("\t\n\r   ", { sep(), eof() });
    test_tokenizer("\nid  ", { sep(), identifier("id", 2, 1), eof() });
    test_tokenizer("3.14", { literal("3.14"), eof() });
    test_tokenizer("=\n", { op("="), sep(), eof() });
    test_tokenizer("\thello 42", { identifier("hello", 1, 8), literal("42"), eof() });
    test_tokenizer("\nx + 1e3 = 20", { sep(), identifier("x"), op("+"), literal("1e3", 2, 5), op("="), literal("20"), eof() });
    test_tokenizer("1+2", { literal("1"), op("+"), literal("2"), eof()});

    const char* const program = R"(
        vals       = 2000
        valsize    = 8
        freq       = 1
        bspersec   = vals * valsize * freq
        bsperday   = bspersec * 60 * 60 * 24
        )";
    test_tokenizer(program, {
            sep(),
            identifier("vals"),     op("="), literal("2000"), sep(),
            identifier("valsize"),  op("="), literal("8"), sep(),
            identifier("freq"),     op("="), literal("1"), sep(),
            identifier("bspersec"), op("="), identifier("vals"), op("*"), identifier("valsize"), op("*"), identifier("freq"), sep(),
            identifier("bsperday"), op("="), identifier("bspersec"), op("*"), literal("60"), op("*"), literal("60"), op("*"), literal("24"), sep(),
            eof(),
            });
}

#include <iostream>
#include <iomanip>
#include <initializer_list>
#include <assert.h>
#include "lex.h"

class token_test {
public:
    static void run() {
        test_tokenizer("",{eof()});
        test_tokenizer("\t\n\r   ",{eof()});
        test_tokenizer(" id  ",{identifier("id"), eof()});
        test_tokenizer("3.14",{literal("3.14"), eof()});
        test_tokenizer("=\n",{op("="), eof()});
        test_tokenizer("\thello 42",{identifier("hello"), literal("42"), eof()});
        test_tokenizer("\tx + 1e3 = 20",{identifier("x"), op("+"), literal("1e3"), op("="), literal("20"), eof()});

        const char* const program = R"(
            vals       = 2000
            valsize    = 8
            freq       = 1
            bspersec   = vals * valsize * freq
            bsperday   = bspersec * 60 * 60 * 24
            )";
        test_tokenizer(program, {
                identifier("vals"),     op("="), literal("2000"),
                identifier("valsize"),  op("="), literal("8"),
                identifier("freq"),     op("="), literal("1"),
                identifier("bspersec"), op("="), identifier("vals"), op("*"), identifier("valsize"), op("*"), identifier("freq"),
                identifier("bsperday"), op("="), identifier("bspersec"), op("*"), literal("60"), op("*"), literal("60"), op("*"), literal("24"),
                eof(),
                });
    }

private:
    static lex::token eof() { return lex::token{}; };
    static lex::token identifier(const char* str) {
        return lex::token{lex::token_type::identifier, str, strlen(str)};
    };
    static lex::token literal(const char* str) {
        return lex::token{lex::token_type::literal, str, strlen(str)};
    };
    static lex::token op(const char* str) {
        return lex::token{lex::token_type::op, str, strlen(str)};
    };

    static void test_tokenizer(const std::string& text, std::initializer_list<lex::token> expected_tokens) {
        lex::tokenizer t{text.c_str(), text.length()};
        for (const auto& expected_token : expected_tokens) {
            auto current_token = t.consume();
            std::cout << current_token << " " << std::flush;
            if (current_token != expected_token) {
                std::cerr << "Tokenizer error. Expeceted " << expected_token << " got " << current_token << std::endl;
                abort();
            }
        }
        assert(t.current().type() == lex::token_type::eof);
        std::cout << "OK!\n";
    }
};

int main()
{
    token_test::run();
}

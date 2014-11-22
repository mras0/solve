#include <iostream>
#include <iomanip>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <assert.h>
#include "lex.h"

class token_test {
public:
    static void run() {
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

private:
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

    static expected_token sep(size_t line=0, size_t col=0) { return expected_token{lex::token_type::separator, "\n", line, col}; };
    static expected_token eof(size_t line=0, size_t col=0) { return expected_token{lex::token_type::eof, "", line, col}; };
    static expected_token identifier(const char* str, size_t line=0, size_t col=0) {
        return expected_token{lex::token_type::identifier, str, line, col};
    };
    static expected_token literal(const char* str, size_t line=0, size_t col=0) {
        return expected_token{lex::token_type::literal, str, line, col};
    };
    static expected_token op(const char* str, size_t line=0, size_t col=0) {
        return expected_token{lex::token_type::op, str, line, col};
    };

    static void test_tokenizer(const std::string& text, std::initializer_list<expected_token> expected_tokens) {
        source::file src{text, text};
        lex::tokenizer t{src};
        for (const auto& expected_token : expected_tokens) {
            auto current_token = t.consume();
            //std::cout << current_token << " " << std::flush;
            expected_token.verify(current_token);
        }
        assert(t.current().type() == lex::token_type::eof);
    }
};

namespace ast {

class expression {
public:
    virtual ~expression() {}
    virtual std::string repr() const = 0;
    virtual const lex::token& start_token() const = 0;
    virtual const lex::token& end_token() const = 0;
};

class literal_node : public expression {
public:
    literal_node(const lex::token& token) : token_(token) {
        assert(token_.type() == lex::token_type::literal);
    }

    virtual std::string repr() const override { return "{literal " + token_.str() + "}"; }
    virtual const lex::token& start_token() const override { return token_; }
    virtual const lex::token& end_token() const override { return token_; }
private:
    lex::token token_;
};

class binary_expression : public expression {
public:
    const expression& lhs() const { return *lhs_; }
    const expression& rhs() const { return *rhs_; }
protected:
    binary_expression(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        assert(lhs_);
        assert(rhs_);
        assert(lhs_->start_token().position() < rhs_->end_token().position());
    }
private:
    std::unique_ptr<expression> lhs_;
    std::unique_ptr<expression> rhs_;
    virtual const lex::token& start_token() const override { return lhs_->start_token(); }
    virtual const lex::token& end_token() const override { return rhs_->end_token(); }
};

struct operator_info {
    const char* repr;
    int precedence;
    bool is_binary;
    bool is_left_associative;

    static const operator_info& get(const std::string& s) {
        static const operator_info op_info[] = {
            { "*", 2, true, true },
            { "/", 2, true, true },
            { "+", 1, true, true },
            { "-", 1, true, true },
        };
        for (const auto& i : op_info) {
            if (s == i.repr) return i;
        }
        throw std::logic_error("Unknown operator \"" + s + "\" in " + __func__);
    }
};

class binary_operation : public binary_expression {
public:
    binary_operation(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs, const operator_info& opinfo) : binary_expression(std::move(lhs), std::move(rhs)) , opinfo_(opinfo) {
        assert(opinfo_.is_binary);
    }
    const operator_info& opinfo() const { return opinfo_; }
    virtual std::string repr() const override { 
        std::ostringstream oss;
        oss << "{" << opinfo_.repr << " " << lhs().repr() << " " << rhs().repr() << "}";
        return oss.str();
    }
private:
    operator_info opinfo_;
};


class parser {
public:
    explicit parser(const source::file& src) : src_(src), tokenizer_(src_) {
    }

    bool eof() {
        return tokenizer_.eof();
    }

    std::unique_ptr<expression> parse_expression() {
        return parse_expression_1(parse_primary_expression(), 0);
    }

private:
    const source::file& src_;
    lex::tokenizer      tokenizer_;

    // http://en.wikipedia.org/wiki/Operator-precedence_parser
    std::unique_ptr<expression> parse_expression_1(std::unique_ptr<expression> lhs, int min_precedence) {
        for (;;) {
            // while lookahead is a binary operator whose precedence is >= min_precedence
            auto lookahead = tokenizer_.current();
            if (lookahead.type() != lex::token_type::op) {
                // Not an operator
                break;
            }
            auto lhs_opinfo = operator_info::get(lookahead.str());

            if (!lhs_opinfo.is_binary) {
                // Not binary
                break;
            }

            if (lhs_opinfo.precedence < min_precedence) {
                // Precedence is not >= min_precedence
                break;
            }

            tokenizer_.consume();

            std::unique_ptr<expression> rhs = parse_primary_expression();
            for (;;) {
                // while lookahead is a binary operator whose precedence is greater
                // than op's, or a right-associative operator whose precedence is equal to op's
                lookahead = tokenizer_.current();
                if (lookahead.type() != lex::token_type::op) {
                    // Not an operator
                    break;
                }
                auto rhs_opinfo = operator_info::get(lookahead.str());

                if (!rhs_opinfo.is_binary) {
                    // Not binary
                    break;
                }

                if (rhs_opinfo.precedence == lhs_opinfo.precedence) {
                    if (rhs_opinfo.is_left_associative) {
                        break;
                    }
                    // right-associative and equal precedence
                } else if (rhs_opinfo.precedence < lhs_opinfo.precedence) {
                    break;
                }

                // the rhs operator has greater precedence (or equal but is right-associative)
                // recurse
                rhs = parse_expression_1(std::move(rhs), rhs_opinfo.precedence);
            }

            // lhs := the result of applying op with operands lhs and rhs
            lhs = std::unique_ptr<expression>{new binary_operation{std::move(lhs), std::move(rhs), lhs_opinfo}};
        }
        return lhs;
    }

    std::unique_ptr<expression> parse_primary_expression() {
        auto tok = tokenizer_.current();
        if (tok.type() == lex::token_type::literal) {
            tokenizer_.consume();
            return std::unique_ptr<expression>(new literal_node{tok});
        }
        throw parse_error("Expeceted literal in parse_primary_expression");
    }

    std::runtime_error parse_error(const std::string& message) {
        const auto& tok = tokenizer_.current();
        std::ostringstream oss;
        oss << "Parse error at " << tok.position() << " (" << tok << " ): " << message;
        return std::runtime_error(oss.str());
    }
};

} // namespace ast

void foo(const ast::expression& expr)
{
    std::cout << "Start = " << expr.start_token().position();
    std::cout << " End = " << expr.end_token().position();
    std::cout << " ==> " << expr.repr() << std::endl;
}

int main()
{
    token_test::run();

    source::file src{"<test>", "1+2*3"};
    ast::parser p{src};
    while (!p.eof()) {
        foo(*p.parse_expression());
    }
}

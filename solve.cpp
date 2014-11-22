#include <iostream>
#include <initializer_list>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <assert.h>
#include "lex.h"

namespace ast {

class expression {
public:
    virtual ~expression() {}
    virtual std::string repr() const = 0;
    virtual const lex::token& start_token() const = 0;
    virtual const lex::token& end_token() const = 0;
};

class literal_expression : public expression {
public:
    literal_expression(const lex::token& token) : token_(token) {
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
            return std::unique_ptr<expression>(new literal_expression{tok});
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

namespace parser_test {

typedef std::function<void (const ast::expression& expr)> expr_verifier;

void print_expression(const ast::expression& expr) {
    std::cout << expr.start_token().position() << " ==> " << expr.repr() << std::endl;
}

expr_verifier lit(double d) {
    return [=](const ast::expression& e) {
        if (auto l = dynamic_cast<const ast::literal_expression*>(&e)) {
            if (std::stod(l->start_token().str()) == d) {
                return;
            }
        }
        std::cout << "Expected literal " << d << " got:" << std::endl;
        print_expression(e);
        assert(false);
    };
}

expr_verifier bin_op(char op, const expr_verifier& lhs, const expr_verifier& rhs) {
    return [=](const ast::expression& e) {
        if (auto l = dynamic_cast<const ast::binary_operation*>(&e)) {
            if (l->opinfo().repr[0] == op && l->opinfo().repr[1] == 0) {
                lhs(l->lhs());
                rhs(l->rhs());
                return;
            }
        }
        std::cout << "Expected binary operator " << op << " got:" << std::endl;
        print_expression(e);
        assert(false);
    };
}

void run_one(const char* name, const char* expr, const expr_verifier& v) {
    source::file src{name, expr};
    ast::parser p{src};
    v(*p.parse_expression());
    assert(p.eof());
}

void run()
{
    run_one("single literal", "3.141592", lit(3.141592));
    run_one("simple bin op", "1\t\r+ 2", bin_op('+', lit(1), lit(2)));
    run_one("precedence test", "1+2*3", bin_op('+', lit(1), bin_op('*', lit(2), lit(3))));
}

} // namespace parser_test

int main()
{
    extern void lex_test();
    lex_test();
    parser_test::run();
}

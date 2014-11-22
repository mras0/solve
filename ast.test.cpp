#include "ast.h"
#include <iostream>
#include <functional>
#include <initializer_list>
#include <assert.h>

namespace {

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

expr_verifier atom(const std::string& s) {
    return [=](const ast::expression& e) {
        if (auto l = dynamic_cast<const ast::atom_expression*>(&e)) {
            if (l->start_token().str() == s) {
                return;
            }
        }
        std::cout << "Expected atom " << s << " got:" << std::endl;
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

void drain(const std::string& name, ast::parser& p)
{
    if (p.eof()) {
        return;
    }
    std::cout << "Expected EOF while running '" << name << "' Still to process:" << std::endl;
    while (!p.eof()) {
        print_expression(*p.parse_expression());
    }
    assert(false);
}

void run_one(const std::string& name, const char* expr, const expr_verifier& v) {
    source::file src{name, expr};
    ast::parser p{src};
    v(*p.parse_expression());
    drain(name, p);
}

void run_many(const char* name, const char* expr, std::initializer_list<expr_verifier> vs) {
    source::file src{name, expr};
    ast::parser p{src};
    for (const auto& v : vs) {
        v(*p.parse_expression());
    }
    drain(name, p);
}

} // unnamed namespace

void ast_test()
{
    run_one("single literal", "3.141592", lit(3.141592));
    run_one("simple bin op", "1\t\r+ 2", bin_op('+', lit(1), lit(2)));
    run_one("precedence test", "1+2*3", bin_op('+', lit(1), bin_op('*', lit(2), lit(3))));
    run_one("equal precedence", "1+2-3", bin_op('-', bin_op('+', lit(1), lit(2)), lit(3)));
    run_one("all ops", "2/3*4-5+6", bin_op('+',bin_op('-', bin_op('*', bin_op('/', lit(2), lit(3)), lit(4)), lit(5)), lit(6)));
    run_one("atom test", "x", atom("x"));
    run_one("atom op lit", "hello+4e3", bin_op('+', atom("hello"), lit(4000)));

    run_many("multiple lines", R"(
        2+xx
        42-60
    )", {
        bin_op('+', lit(2), atom("xx")),
        bin_op('-', lit(42), lit(60)),
    });
#if 0
    run_many("program", R"(
        vals       = 2000
        valsize    = 8
        freq       = 1
        bspersec   = vals * valsize * freq
        bsperday   = bspersec * 60 * 60 * 24
    )", {
    });
#endif
}

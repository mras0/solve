#include "ast.h"
#include <iostream>
#include <functional>
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

void run_one(const char* name, const char* expr, const expr_verifier& v) {
    source::file src{name, expr};
    ast::parser p{src};
    v(*p.parse_expression());
    assert(p.eof());
}

} // unnamed namespace

void ast_test()
{
    run_one("single literal", "3.141592", lit(3.141592));
    run_one("simple bin op", "1\t\r+ 2", bin_op('+', lit(1), lit(2)));
    run_one("precedence test", "1+2*3", bin_op('+', lit(1), bin_op('*', lit(2), lit(3))));
    run_one("atom test", "x", atom("x"));
}

#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <assert.h>
#include "ast.h"

void test()
{
    extern void lex_test();
    extern void ast_test();
    lex_test();
    ast_test();
}

template<typename Derived>
struct ast_visitor {
protected:
    template<typename Arg>
    bool try_visit(const ast::expression& arg, void (Derived::* f)(const Arg&)) {
        if (auto a = dynamic_cast<const Arg*>(&arg)) {
            (static_cast<Derived&>(*this).*f)(*a);
            return true;
        }
        return false;
    }
};

struct visitor : public ast_visitor<visitor> {

    void do_it(const ast::expression& e) {
        try_visit(e, &visitor::visit_bin_op);
        std::cout << "In " << __func__ << " e: " << e.repr() << std::endl;
        assert(false);
    }

    void visit_bin_op(const ast::binary_operation& e) {
        if (e.op() == '=') {
            std::cout << "Handle bind of " << e.lhs().repr() << " and " << e.rhs().repr() << "\n";
        }
    }
};

int main()
{
    test();

    const char* const program = R"(
        2 * x = 8
        )";
    source::file src{"<example>", program};
    ast::parser p{src};
    std::vector<std::unique_ptr<ast::expression>> exprs;
    while (!p.eof()) {
        exprs.emplace_back(p.parse_expression());
    }
    visitor v;
    for (const auto& ep : exprs) {
        v.do_it(*ep);
    }
}

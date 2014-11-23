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
    for (const auto& ep : exprs) {
        if (auto bin_op = dynamic_cast<const ast::binary_operation*>(ep.get())) {
            if (bin_op->op() == '=') {
                std::cout << "Handle bind of " << bin_op->lhs().repr() << " and " << bin_op->rhs().repr() << "\n";
                continue;
            }
        }
        std::cout << "Unexpected top level expression:\n" << ep->repr() << std::endl;
        assert(false);
    }
}

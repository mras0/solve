#include "ast.h"
#include <assert.h>
#include <sstream>
#include <stdexcept>

namespace ast {

literal_expression::literal_expression(const lex::token& token) : token_(token) {
    assert(token_.type() == lex::token_type::literal);
}

atom_expression::atom_expression(const lex::token& token) : token_(token) {
    assert(token_.type() == lex::token_type::identifier);
}

binary_expression::binary_expression(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    assert(lhs_);
    assert(rhs_);
    assert(lhs_->start_token().position() < rhs_->end_token().position());
}

binary_operation::binary_operation(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs, const operator_info& opinfo) : binary_expression(std::move(lhs), std::move(rhs)) , opinfo_(opinfo) {
    assert(opinfo_.is_binary);
}

std::string binary_operation::repr() const {
    std::ostringstream oss;
    oss << "{" << opinfo_.repr << " " << lhs().repr() << " " << rhs().repr() << "}";
    return oss.str();
}


// http://en.wikipedia.org/wiki/Operator-precedence_parser
std::unique_ptr<expression> parser::parse_expression_1(std::unique_ptr<expression> lhs, int min_precedence) {
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

std::unique_ptr<expression> parser::parse_primary_expression() {
    auto tok = tokenizer_.current();
    if (tok.type() == lex::token_type::literal) {
        tokenizer_.consume();
        return std::unique_ptr<expression>(new literal_expression{tok});
    } else if (tok.type() == lex::token_type::identifier) {
        tokenizer_.consume();
        return std::unique_ptr<expression>(new atom_expression{tok});
    }
    throw parse_error("Expeceted literal or atom in parse_primary_expression");
}

std::runtime_error parser::parse_error(const std::string& message) {
    const auto& tok = tokenizer_.current();
    std::ostringstream oss;
    oss << "Parse error at " << tok.position() << " (" << tok << " ): " << message;
    return std::runtime_error(oss.str());
}

} // namespace ast

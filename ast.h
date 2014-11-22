#ifndef SOLVE_AST_H
#define SOLVE_AST_H

#include <string>
#include <memory>
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
    literal_expression(const lex::token& token);

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
    binary_expression(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs);
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
    binary_operation(std::unique_ptr<expression> lhs, std::unique_ptr<expression> rhs, const operator_info& opinfo);
    const operator_info& opinfo() const { return opinfo_; }
    virtual std::string repr() const override;
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

    std::unique_ptr<expression> parse_expression_1(std::unique_ptr<expression> lhs, int min_precedence);
    std::unique_ptr<expression> parse_primary_expression();
    std::runtime_error parse_error(const std::string& message);
};

} // namespace ast



#endif

#include <iostream>
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

class expr {
public:
    virtual ~expr() {}
    virtual std::unique_ptr<expr> clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const expr& e) {
        e.print(os);
        return os;
    }

protected:
    expr() {}

private:
    virtual void print(std::ostream& os) const = 0;
};

typedef std::unique_ptr<expr> expr_ptr;

class const_expr : public expr {
public:
    explicit const_expr(double value) : value_(value) {}
    virtual std::unique_ptr<expr> clone() const override { return std::unique_ptr<expr>{new const_expr{value_}}; }
    double value() const { return value_; }
private:
    double value_;
    virtual void print(std::ostream& os) const override {
        os << value_;
    }
};

class var_expr : public expr {
public:
    explicit var_expr(const std::string& name) : name_(name) {}
    std::string name() const { return name_; }
    virtual std::unique_ptr<expr> clone() const override { return std::unique_ptr<expr>{new var_expr{name_}}; }
private:
    std::string name_;
    virtual void print(std::ostream& os) const override {
        os << name_;
    }
};

class bin_op_expr : public expr {
public:
    bin_op_expr(expr_ptr lhs, expr_ptr rhs, char op) : lhs_(std::move(lhs)), rhs_(std::move(rhs)), op_(op) {}
    const expr& lhs() const { return *lhs_; }
    const expr& rhs() const { return *rhs_; }
    char op() const { return op_; }
    virtual std::unique_ptr<expr> clone() const override {
        return std::unique_ptr<expr>{new bin_op_expr{lhs().clone(), rhs().clone(), op_}};
    }
private:
    expr_ptr lhs_;
    expr_ptr rhs_;
    char op_;
    virtual void print(std::ostream& os) const override {
        os << "{" << op_ << " " << lhs() << " " << rhs() << "}";
    }
};

expr_ptr constant(double d) { return expr_ptr{new const_expr{d}}; }
expr_ptr var(const std::string& n) { return expr_ptr{new var_expr{n}}; }

expr_ptr operator+(expr_ptr a, expr_ptr b) {
    return expr_ptr{new bin_op_expr{std::move(a), std::move(b), '+'}};
}

expr_ptr operator-(expr_ptr a, expr_ptr b) {
    return expr_ptr{new bin_op_expr{std::move(a), std::move(b), '-'}};
}

expr_ptr operator*(expr_ptr a, expr_ptr b) {
    return expr_ptr{new bin_op_expr{std::move(a), std::move(b), '*'}};
}

expr_ptr operator/(expr_ptr a, expr_ptr b) {
    return expr_ptr{new bin_op_expr{std::move(a), std::move(b), '/'}};
}

std::ostream& operator<<(std::ostream& os, const expr_ptr& e) {
    os << *e;
    return os;
}

template<typename T>
const T* expr_cast(const expr_ptr& e) {
    return dynamic_cast<const T*>(e.get());
}

bool match_const(const expr_ptr& e, const double& v) {
    auto cp = expr_cast<const_expr>(e);
    return cp && cp->value() == v;
}

bool match_var(const expr_ptr& e, const std::string& v) {
    auto vp = expr_cast<var_expr>(e);
    return vp && vp->name() == v;
}

// Solve the equation "e" for variable "v"
expr_ptr solve(const expr_ptr& lhs, const expr_ptr& rhs, const std::string& v) {
    if (match_var(lhs, v)) {
        return rhs->clone();
    }
    if (match_var(rhs, v)) {
        return lhs->clone();
    }

    std::ostringstream oss;
    oss << "Unable to solve '" << lhs << "'='" << rhs << "' for '" << v << "'";
    throw std::runtime_error(oss.str());
}

void test_solve(const expr_ptr& lhs, const expr_ptr& rhs, const std::string& v, double value) {
    auto s = solve(lhs, rhs, v);
    if (match_const(s, value)) {
        return;
    }

    std::cout << "Wrong answer for '" << lhs << "'='" << rhs << "' for '" << v << "'\n";
    std::cout << "Expected: " << value << std::endl;
    std::cout << "Got: '" << s << "'" << std::endl;
}


int main()
{
    test();

    test_solve(var("x"), constant(8), "x", 8);
    test_solve(constant(42), var("x"), "x", 42);
//    std::cout << solve(constant(2) * var("x"), constant(8), "x") << std::endl;
}

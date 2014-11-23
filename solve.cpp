#include <iostream>
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

    friend std::ostream& operator<<(std::ostream& os, const expr& e) {
        e.print(os);
        return os;
    }

protected:
    expr() {}

private:
    virtual void print(std::ostream& os) const = 0;
};

class const_expr : public expr {
public:
    explicit const_expr(double value) : value_(value) {}
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
private:
    std::string name_;
    virtual void print(std::ostream& os) const override {
        os << name_;
    }
};

class bin_op_expr : public expr {
public:
    bin_op_expr(std::unique_ptr<expr> lhs, std::unique_ptr<expr> rhs, char op) : lhs_(std::move(lhs)), rhs_(std::move(rhs)), op_(op) {}
    const expr& lhs() const { return *lhs_; }
    const expr& rhs() const { return *rhs_; }
    char op() const { return op_; }
private:
    std::unique_ptr<expr> lhs_;
    std::unique_ptr<expr> rhs_;
    char op_;
    virtual void print(std::ostream& os) const override {
        os << "{" << op_ << " " << lhs() << " " << rhs() << "}";
    }
};

std::unique_ptr<expr> constant(double d) { return std::unique_ptr<expr>{new const_expr{d}}; }
std::unique_ptr<expr> var(const std::string& n) { return std::unique_ptr<expr>{new var_expr{n}}; }

std::unique_ptr<expr> operator+(std::unique_ptr<expr> a, std::unique_ptr<expr> b) {
    return std::unique_ptr<expr>{new bin_op_expr{std::move(a), std::move(b), '+'}};
}

std::unique_ptr<expr> operator-(std::unique_ptr<expr> a, std::unique_ptr<expr> b) {
    return std::unique_ptr<expr>{new bin_op_expr{std::move(a), std::move(b), '-'}};
}

std::unique_ptr<expr> operator*(std::unique_ptr<expr> a, std::unique_ptr<expr> b) {
    return std::unique_ptr<expr>{new bin_op_expr{std::move(a), std::move(b), '*'}};
}

std::unique_ptr<expr> operator/(std::unique_ptr<expr> a, std::unique_ptr<expr> b) {
    return std::unique_ptr<expr>{new bin_op_expr{std::move(a), std::move(b), '/'}};
}

std::ostream& operator<<(std::ostream& os, const std::unique_ptr<expr>& e) {
    os << *e;
    return os;
}

int main()
{
    test();

    std::cout << (constant(10) + var("x")) << std::endl;
}

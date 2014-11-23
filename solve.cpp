#include <iostream>
#include <sstream>
#include <queue>
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

    operator std::unique_ptr<expr>() const {
        return clone();
    }

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

template<typename T, typename E>
const T* expr_cast(const E& e) {
    return dynamic_cast<const T*>(&e);
}

bool match_const(const expr& e, const double& v) {
    auto cp = expr_cast<const_expr>(e);
    return cp && cp->value() == v;
}

bool extract_const(const expr& e, double& v) {
    if (auto cp = expr_cast<const_expr>(e)) {
        v = cp->value();
        return true;
    }
    v = 0;
    return false;
}

bool match_var(const expr& e, const std::string& v) {
    auto vp = expr_cast<var_expr>(e);
    return vp && vp->name() == v;
}

struct solver {
    explicit solver(const std::string& v) : v_(v) {}

    expr_ptr solve(const expr& lhs, const expr& rhs) {
        items_ = std::queue<item_type>{};
        push_job(lhs.clone() - rhs.clone(), constant(0));
        return do_solve_1();
    }

    // Solve the equation "e" for variable "v"
private:
    typedef std::pair<expr_ptr, expr_ptr> item_type;
    std::string v_;
    std::queue<item_type> items_;

    void push_job(expr_ptr lhs, expr_ptr rhs) {
        items_.push(make_pair(std::move(lhs), std::move(rhs)));
    }

    expr_ptr do_solve_1() {
        while (!items_.empty()) {
            auto& top = items_.front();
            if (auto e = do_solve(*top.first, *top.second)) {
                return e;
            }
            items_.pop();
        }
        assert(false);
        return nullptr;
    }

    expr_ptr do_solve(const expr& lhs, const expr& rhs) {
        double val;
        std::cout << ">>>solve lhs=" << lhs << " rhs=" << rhs << std::endl;
        if (match_var(lhs, v_) && extract_const(rhs, val)) {
            return constant(val);
        }
        if (match_var(rhs, v_) && extract_const(lhs, val)) {
            return constant(val);
        }
        if (auto bp = expr_cast<bin_op_expr>(lhs)) {
            return do_solve_bin_op(*bp, rhs);
        }
        if (auto bp = expr_cast<bin_op_expr>(rhs)) {
            return do_solve_bin_op(*bp, lhs);
        }
        assert(false);
        return nullptr;
    }

    expr_ptr do_solve_bin_op(const bin_op_expr& a, const expr& b) {
        double ac, bc;
        if (extract_const(a.lhs(), ac) && extract_const(a.rhs(), bc)) {
            switch (a.op()) {
                case '+': return constant(ac + bc);
                case '-': return constant(ac - bc);
                case '*': return constant(ac * bc);
                case '/': return constant(ac / bc);
                default:
                    std::cout << "Don't know how to handle " << a.op() << std::endl;
                    assert(false);
            }
        }

        switch (a.op()) {
            case '+':
                push_job(a.lhs(), b - a.rhs());
                push_job(a.rhs(), b - a.lhs());
                break;
            case '-':
                push_job(a.lhs(), b + a.rhs());
                push_job(a.rhs(), b + a.lhs());
                break;
             case '*':
                push_job(a.lhs(), b / a.rhs());
                push_job(a.rhs(), b / a.lhs());
                break;
            case '/':
                push_job(a.lhs(), b * a.rhs());
                push_job(a.rhs(), b * a.lhs());
                break;
            default:
                std::cout << "Don't know how to handle " << a.op() << std::endl;
                assert(false);
        }
        return nullptr;
    }
};

void test_solve(const expr_ptr& lhs, const expr_ptr& rhs, const std::string& v, double value) {
    solver solv{v};
    auto s = solv.solve(*lhs, *rhs);
    if (!s) {
        std::cout << "Unable to solve '" << lhs << "'='" << rhs << "' for '" << v << "'" << std::endl;
        assert(false);
    }
    if (match_const(*s, value)) {
        std::cout << "OK: " << lhs << "=" << rhs << " ==> " << v << "=" << *s << std::endl;
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
    test_solve(constant(2) * var("x"), constant(8), "x", 4);
    // TODO:
    // - Normalize expression: lhs = rhs -> lhs - rhs = 0 and work from there
    // - Prioritize jobs by "badness"[not good] ("clean" rhs good? less tree depth good?)
    // - Avoid doing the same work again in push_job (hash_map of finished jobs)
    // - Isolate each atom in turn, when find(lhs, *match_atom(rhs, the_atom)) returns false we're done
}

#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_set>
#include <functional>
#include <assert.h>
#include "ast.h"

size_t hash_combine(size_t a, size_t b) {
    // blah, blah, terrible, etc. etc.
    return a ^ b;
}

class expr {
public:
    virtual ~expr() {}
    virtual std::unique_ptr<expr> clone() const = 0;
    virtual size_t hash() const = 0;
    virtual bool equal(const expr& e) const = 0;

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

template<typename T, typename E>
const T* expr_cast(const E& e) {
    return dynamic_cast<const T*>(&e);
}


class const_expr : public expr {
public:
    explicit const_expr(double value) : value_(value) {}
    virtual std::unique_ptr<expr> clone() const override { return std::unique_ptr<expr>{new const_expr{value_}}; }
    double value() const { return value_; }
    virtual size_t hash() const override { return std::hash<double>()(value_); }
    virtual bool equal(const expr& e) const override { auto ep = expr_cast<const_expr>(e); return ep && ep->value() == value(); }
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
    virtual size_t hash() const override { return std::hash<std::string>()(name_); }
    virtual bool equal(const expr& e) const override { auto ep = expr_cast<var_expr>(e); return ep && ep->name() == name(); }
private:
    std::string name_;
    virtual void print(std::ostream& os) const override {
        os << name_;
    }
};

class negation_expr : public expr {
public:
    explicit negation_expr(expr_ptr e) : e_(std::move(e)) {
    }
    const expr& e() const { return *e_; }
    virtual std::unique_ptr<expr> clone() const override {
        return std::unique_ptr<expr>{new negation_expr{e().clone()}};
    }
    virtual size_t hash() const override {
        return hash_combine('-', e().hash());
    }
    virtual bool equal(const expr& other) const override {
        auto ep = expr_cast<negation_expr>(other);
        if (!ep) return false;
        return e().equal(ep->e());
    //    if (!ep->e().equal(e())) return false;
        return true;
    }
private:
    expr_ptr e_;
    virtual void print(std::ostream& os) const override {
        os << "-(" << e() << ")";
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
    virtual size_t hash() const override {
        return hash_combine(hash_combine(op(), lhs().hash()), rhs().hash());
    }
    virtual bool equal(const expr& e) const override {
        auto ep = expr_cast<bin_op_expr>(e);
        if (!ep) return false;
        if (ep->op() != op()) return false;
        if (!ep->lhs().equal(lhs())) return false;
        if (!ep->rhs().equal(rhs())) return false;
        return true;
    }
private:
    expr_ptr lhs_;
    expr_ptr rhs_;
    char op_;
    virtual void print(std::ostream& os) const override {
        os << "(" << lhs() << " " << op() << " " << rhs() << ")";
    }
};

expr_ptr constant(double d) { return expr_ptr{new const_expr{d}}; }
expr_ptr var(const std::string& n) { return expr_ptr{new var_expr{n}}; }

expr_ptr operator-(expr_ptr e) {
    return expr_ptr{new negation_expr{std::move(e)}};
}

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

expr_ptr simplify(expr_ptr e) {
    double ac, bc;
    auto bp = expr_cast<bin_op_expr>(*e);
    if (bp) {
        if (extract_const(bp->lhs(), ac) && extract_const(bp->rhs(), bc)) {
            switch (bp->op()) {
                case '+': return constant(ac + bc);
                case '-': return constant(ac - bc);
                case '*': return constant(ac * bc);
                case '/': return constant(ac / bc);
                default:
                          std::cout << "Don't know how to handle " << *bp << std::endl;
                          assert(false);
            }
        }
        if (match_const(bp->lhs(), 0)) {
            switch (bp->op()) {
                case '+': return bp->rhs();
                          //case '-': return constant(ac - bc);
                          //case '*': return constant(ac * bc);
                          //case '/': return constant(ac / bc);
                default:
                          std::cout << "Don't know how to handle " << *bp << std::endl;
                          assert(false);
            }
        }
        if (match_const(bp->rhs(), 0)) {
            assert(false);
        }
    }
    return e;
}

typedef std::pair<expr_ptr, expr_ptr> job_type;
const job_type empty_job{nullptr, nullptr};

// std::hash<> specialization for job_type
namespace std {
template<>
struct hash<job_type> {
    size_t operator()(const job_type& i) const {
        return hash_combine(i.first->hash(), i.second->hash());
    }
};
} // namespace std

bool operator==(const job_type& a, const job_type& b) {
    return a.first->equal(*b.first) && a.second->equal(*b.second);
}

std::ostream& operator<<(std::ostream& os, const job_type& j) {
    return os << "{job " << *j.first << " " << *j.second << "}";
}

class job_list {
public:
    explicit job_list() {}

    void add(expr_ptr lhs, expr_ptr rhs) {
        assert(lhs && rhs);
        auto job = make_pair(simplify(std::move(lhs)), simplify(std::move(rhs)));
        if (old_items_.find(job) != old_items_.end()) {
            //std::cout << "skipping " << job << std::endl;
            return;
        }
        std::cout << "> new job " << job << std::endl;
        auto res = old_items_.emplace(std::move(job));
        assert(res.second && "item already found in old_items_");
        items_.push(&*res.first);
    }

    const job_type& next() {
        if (items_.empty()) {
            return empty_job;
        }
        const auto& job = *items_.front();
        items_.pop();
        return job;
    }

private:
    std::queue<const job_type*> items_;
    std::unordered_set<job_type> old_items_;
};


// Solve the equation "e" for variable "v"
class solver {
public:
    explicit solver(const std::string& v) : v_(v) {}

    expr_ptr solve(const expr& lhs, const expr& rhs) {
        items_ = job_list{};
        items_.add(lhs.clone(), rhs.clone());
        return do_solve_all();
    }

private:
    std::string v_;
    job_list    items_;

    expr_ptr do_solve_all() {
        for (;;) {
            const auto& job = items_.next();
            if (job == empty_job) {
                break;
            }
            assert(job.first && job.second);
            if (auto e = do_solve(*job.first, *job.second)) {
                return e;
            }
        }
        return nullptr;
    }

    expr_ptr do_solve(const expr& lhs, const expr& rhs) {
        double val;
        std::cout << ">>> solve lhs=" << lhs << " rhs=" << rhs << std::endl;
        if (match_var(lhs, v_) && extract_const(rhs, val)) {
            return constant(val);
        }
        if (match_var(rhs, v_) && extract_const(lhs, val)) {
            return constant(val);
        }
        if (auto np = expr_cast<negation_expr>(lhs)) {
            return do_solve_neg(*np, rhs);
        }
        if (auto np = expr_cast<negation_expr>(rhs)) {
            return do_solve_neg(*np, lhs);
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

    // Rewrite {-n.e, r} to {n.e, -r}
    expr_ptr do_solve_neg(const negation_expr& n, const expr& r) {
        items_.add(n.e(), -r);
        return nullptr;
    }

    // Rewrite {a.lhs OP a.rhs, b} using our knowledge of "OP"
    expr_ptr do_solve_bin_op(const bin_op_expr& a, const expr& b) {
        auto& l = a.lhs();
        auto& r = a.rhs();
        std::cout << "> solve bin op " << l << " " << a.op() << " " << r << " = " << b << std::endl;
        switch (a.op()) {
            case '+':
                // { L + R, B } -> { L, B - R } and { R, B - L }
                items_.add(l, b - r);
                items_.add(r, b - l);
                break;
            case '-':
                // { L - R, B } -> { L, B + R } and {  -R, B - L }
                items_.add(l, b + r);
                items_.add(-r, b - l);
                break;
            case '*':
                // { L * R, B } -> { L, B / R } and { R, B / L }
                items_.add(l, b / r);
                items_.add(r, b / l);
                break;
            case '/':
                // { L / R, B } -> { L, B * R } and { 1 / R, B / L }
                items_.add(l, b * r);
                items_.add(constant(1) / r, b / l);
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
    assert(false);
}

// TODO:
// - Prioritize jobs by "badness"[not good] ("clean" rhs good? less tree depth good?)
// - Isolate each atom in turn, when find(lhs, *match_atom(rhs, the_atom)) returns false we're done
// - Take advantage of symmetry (i.e. L=R and R=L are equivalent)
// - Make simplification rules table based or allow them to be specified in a DSL
//
int main()
{
    extern void lex_test();
    extern void ast_test();
    lex_test();
    ast_test();
    test_solve(var("x"), constant(8), "x", 8);
    test_solve(constant(42), var("x"), "x", 42);
    test_solve(constant(2) * var("x"), constant(8), "x", 4);
    test_solve(constant(3) + constant(60) / var("zz"), constant(6), "zz", 20);
    //test_solve(var("x") * constant(4) + constant(10), var("y"), "x", 0);
    //test_solve(var("x") * constant(4), var("y"), "x", 0);
}

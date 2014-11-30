#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_set>
#include <set>
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

////////////////////////////
// SIMPLIFY
////////////////////////////

#include <tuple>
#include <type_traits>

template<typename A>
struct const_matcher {
public:
    typedef typename std::result_of<A(double)>::type result_type;
    const_matcher(const A& a) : a_(a) {}
    result_type operator()(const expr& e) {
        if (auto ce = expr_cast<const_expr>(e)) {
            return a_(ce->value());
        }
        return result_type{};
    }
private:
    A a_;
};

template<typename A>
struct neg_matcher {
    typedef typename std::result_of<A(const expr&)>::type result_type;
    neg_matcher(const A& a) : a_(a) {}
    result_type operator()(const expr& e) {
        if (auto ne = expr_cast<negation_expr>(e)) {
            return a_(ne->e());
        }
        return result_type{};
    }
private:
    A a_;
};

template<typename A>
struct var_matcher {
    typedef typename std::result_of<A(const std::string&)>::type result_type;
    var_matcher(const A& a) : a_(a) {}
    result_type operator()(const expr& e) {
        if (auto ve = expr_cast<var_expr>(e)) {
            return a_(ve->name());
        }
        return result_type{};
    }
private:
    A a_;
};


template<typename A>
struct bin_op_matcher {
    typedef typename std::result_of<A(const bin_op_expr&)>::type result_type;
    bin_op_matcher(const A& a) : a_(a) {}
    result_type operator()(const expr& e) {
        if (auto be = expr_cast<bin_op_expr>(e)) {
            return a_(*be);
        }
        return result_type{};
    }
private:
    A a_;
};

template<typename A, typename... As>
struct or_matcher {
    typedef typename std::result_of<A(const expr&)>::type result_type;

    or_matcher(const A& a, const As&... as) : as_(a, as...) {
    }

    result_type operator()(const expr& e) {
        return iter_helper<0>(as_, e);
    }

private:
    typedef std::tuple<A, As...> tuple_type;
    tuple_type as_;

    template<int N>
    static typename std::enable_if<N < std::tuple_size<tuple_type>::value, result_type>::type
    iter_helper(tuple_type& t, const expr& e) {
        auto& a = std::get<N>(t);
        if (auto res = a(e)) {
            return res;
        }
        return iter_helper<N+1>(t, e);
    }
    template<int N>
    static typename std::enable_if<N == std::tuple_size<tuple_type>::value, result_type>::type
    iter_helper(tuple_type&, const expr&) {
        return result_type{};
    }
};

template<typename T>
struct binder {
    binder(T& x) : x_(&x), bound_(false) {
    }
    bool operator()(const T& x) {
        assert(!bound_);
        *x_ = x;
        bound_ = true;
        return true;
    }
    bool bound() const { return bound_; }
private:
    T*   x_;
    bool bound_;
};

template<typename T>
binder<T> binder_m(T& x) { return binder<T>(x); }

template<typename A>
const_matcher<A> const_m(const A& a) { return const_matcher<A>(a); }

template<typename A>
var_matcher<A> var_m(const A& a) { return var_matcher<A>(a); }

template<typename A>
neg_matcher<A> neg_m(const A& a) { return neg_matcher<A>(a); }

template<typename A>
bin_op_matcher<A> bin_op_m(const A& a) { return bin_op_matcher<A>(a); }

template<typename A, typename... As>
or_matcher<A, As...> or_m(const A& a, const As&... as) {
    return or_matcher<A, As...>(a, as...);
}

expr_ptr simplify(const expr& e);

expr_ptr simplify_bin_const_const(char op, double l, double r) {
    switch (op) {
    case '+': return constant(l + r);
    case '-': return constant(l - r);
    case '*': return constant(l * r);
    case '/': return constant(l / r);
    }
    std::cout << "Don't know how to handle " << op << std::endl;
    assert(false);
}

expr_ptr simplify_bin_const_expr(char op, double l, const expr& e) {
    switch (op) {
    case '+':
        if (l == 0.0) return e;
        break;
    case '-':
        if (l == 0.0) return -e;
        break;
    case '*':
        if (l == 0.0) return constant(0);
        if (l == 1.0) return e;
        break;
    case '/':
        if (l == 0.0) return constant(0);
        break;
    default:
        std::cout << "Don't know how to handle " << op << std::endl;
        assert(false);
    }
    return nullptr;
}

expr_ptr simplify_bin_expr_const(char op, const expr& e, double r) {
    switch (op) {
    case '+':
        if (r == 0.0) return e;
        break;
    case '-':
        if (r == 0.0) return e;
        break;
    case '*':
        if (r == 0.0) return constant(0);
        if (r == 1.0) return e;
        break;
    case '/':
        break;
    default:
        std::cout << "Don't know how to handle " << op << std::endl;
        assert(false);
    }
    return nullptr;
}

expr_ptr simplify_bin_op(const bin_op_expr& e) {
    auto lhs = simplify(e.lhs());
    auto rhs = simplify(e.rhs());
    double l, r;
    if (const_m(binder_m(l))(*lhs)) {
        if (const_m(binder_m(r))(*rhs)) {
            return simplify_bin_const_const(e.op(), l, r);
        }
        return simplify_bin_const_expr(e.op(), l, *rhs);
    }
    if (const_m(binder_m(r))(*rhs)) {
        return simplify_bin_expr_const(e.op(), *lhs, r);
    }
    return expr_ptr{};
}

expr_ptr simplify(const expr& e) {
    auto negation_simplification
        = neg_m(or_m(const_m([](double c) { return constant(-c); }),
                        neg_m([](const expr& e) { return simplify(e); })
                       )
                  );
    auto m = or_m(negation_simplification, bin_op_m(&simplify_bin_op));
    if (auto res = m(e)) {
        return res;
    }
    return e;
}

void test_simplify(const expr_ptr& e, const expr_ptr& expected) {
    auto simplified = simplify(*e);
    if (!simplified->equal(*expected)) {
        std::cerr << "Simplification of " << *e << " failed.\n";
        std::cerr << "Expected: " << *expected << "\n";
        std::cerr << "Got: " << *simplified << "\n";
        assert(false);
    }
}

void simplify_test()
{
    const std::pair<expr_ptr,expr_ptr> simplification_tests[] = {
        // Identity
        { constant(2), constant(2) },
        { var("x"), var("x") },
        // Negation
        { -constant(2), constant(-2) },
        { -(-var("x")), var("x") },
        // Constant binary expressions
        { constant(4) + constant(2), constant(6) },
        { constant(3) - constant(5), constant(-2) },
        { constant(10) * constant(2), constant(20) },
        { constant(30) / constant(5), constant(6) },
        // Various identities
        { constant(0) + var("x"), var("x") },
        { var("x") + constant(0), var("x") },
        { constant(0) - var("x"), -var("x") },
        { var("x") - constant(0), var("x") },
        { constant(0) * var("x"), constant(0) },
        { var("x") * constant(0), constant(0) },
        { constant(1) * var("x"), var("x") },
        { var("x") * constant(1), var("x") },
        { constant(0) / var("x"), constant(0) },
        // Some combined tests
        { constant(0) + var("x") * constant(1), var("x") },
        { var("x") * (var("x") * (constant(2) - constant(2))), constant(0) },
    };
    for (const auto& test : simplification_tests) {
        test_simplify(test.first, test.second);
    }
}

////////////////////////////
// JOB LIST
////////////////////////////

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
        auto job = make_pair(simplify(*lhs), simplify(*rhs));
        if (old_items_.find(job) != old_items_.end()) {
            //std::cout << "skipping " << job << std::endl;
            return;
        }
        std::swap(job.first, job.second);
        if (old_items_.find(job) != old_items_.end()) {
            //std::cout << "skipping " << job << " because of symmetry!" << std::endl;
            return;
        }
        std::swap(job.first, job.second);
        auto res = old_items_.emplace(std::move(job));
        assert(res.second && "item already found in old_items_");
        items_.push(&*res.first);

        assert(items_.size() < 100);
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


////////////////////////////
// SOLVER
////////////////////////////

void do_find_vars_in_expr(const expr& e, std::set<std::string>& vars) {
    auto lm = 
        or_m(neg_m([&](const expr& ne) {
                    do_find_vars_in_expr(ne, vars);
                    return true; }),
                bin_op_m([&](const bin_op_expr& be) {
                    do_find_vars_in_expr(be.lhs(), vars);
                    do_find_vars_in_expr(be.rhs(), vars);
                    return true; }),
                const_m([&](double) {
                    return true;
                    }),
                var_m([&](const std::string& name) {
                    vars.insert(name);
                    return true; })
            );

    if (!lm(e)) {
        std::cout << e << std::endl;
        assert(false);
    }
}

std::set<std::string> find_vars_in_expr(const expr& e) {
    std::set<std::string> vars;
    do_find_vars_in_expr(e, vars);
    return vars;
}

bool expr_has_var(const expr& e, const std::string& v) {
    auto vars = find_vars_in_expr(e);
    return vars.find(v) != vars.end();
}

class solver {
public:
    // Solve the equation "lhs = rhs" for variable "v"
    static expr_ptr solve_for(const std::string& v, const expr& lhs, const expr& rhs) {
        solver s{v};
        s.items_.add(lhs.clone(), rhs.clone());
        return s.do_solve_all();
    }

private:
    explicit solver(const std::string& v) : v_(v) {}

    std::string v_;
    job_list    items_;

    expr_ptr do_solve_all() {
        for (;;) {
            const auto& job = items_.next();
            if (job == empty_job) {
                break;
            }
            assert(job.first && job.second);
            const auto& lhs = *job.first;
            const auto& rhs = *job.second;
            std::cout << ">>> solve lhs=" << lhs << " rhs=" << rhs << std::endl;
            if (match_var(lhs, v_) && !expr_has_var(rhs, v_)) {
                return rhs;
            } else if (match_var(rhs, v_) && !expr_has_var(lhs, v_)) {
                return lhs;
            } else {
                do_solve_lhs(lhs, rhs);
                do_solve_lhs(rhs, lhs);
            }
        }
        return nullptr;
    }

    void do_solve_lhs(const expr& lhs, const expr& rhs) {
        auto lm = 
            or_m(neg_m([&](const expr& ne) { items_.add(ne, -rhs); return true; }),
                bin_op_m([&](const bin_op_expr& be) { do_solve_bin_op(be, rhs); return true; }),
                const_m([&](double) { items_.add(constant(0), rhs - lhs); return true; }),
                var_m([&](const std::string&) { items_.add(constant(0), rhs - lhs); return true; })
                );

        if (!lm(lhs)) {
            std::cout << lhs << std::endl;
            assert(false);
        }
    }

    // Rewrite {a.lhs OP a.rhs, b} using our knowledge of "OP"
    void do_solve_bin_op(const bin_op_expr& a, const expr& b) {
        auto& l = a.lhs();
        auto& r = a.rhs();
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
    }
};

std::ostream& operator<<(std::ostream& os, const std::set<std::string>& ss) {
    os << "{";
    for (const auto& s : ss) {
        os << " " << s;
    }
    os << " }";
    return os;
}

void test_find_vars_in_expr(const expr_ptr& e, const std::set<std::string>& expected) {
    auto res = find_vars_in_expr(*e);
    if (res != expected) {
        std::cout << "find_vars_in_expr failed for " << *e << std::endl;
        std::cout << "Expected: " << expected << std::endl;
        std::cout << "Got: " << res << std::endl;
        assert(false);
    }
    for (const auto& var : expected) {
        assert(expr_has_var(*e, var));
    }
}

void test_solve(const expr_ptr& lhs, const expr_ptr& rhs, const std::string& v, const expr_ptr& expected) {
    auto s = solver::solve_for(v, *lhs, *rhs);
    if (!s) {
        std::cout << "Unable to solve '" << lhs << "'='" << rhs << "' for '" << v << "'" << std::endl;
        assert(false);
    }
    if (expected->equal(*s)) {
        std::cout << "OK: " << lhs << "=" << rhs << " ==> " << v << "=" << *s << std::endl;
        return;
    }

    std::cout << "Wrong answer for '" << lhs << "'='" << rhs << "' for '" << v << "'\n";
    std::cout << "Expected: " << expected << std::endl;
    std::cout << "Got: '" << s << "'" << std::endl;
    assert(false);
}
void solve_test()
{
    test_find_vars_in_expr(constant(0), {});
    test_find_vars_in_expr(var("x"), {"x"});
    test_find_vars_in_expr(-var("x"), {"x"});
    test_find_vars_in_expr(var("x")+var("x"), {"x"});
    test_find_vars_in_expr(var("x")+var("y"), {"x","y"});

    test_solve(var("x"), constant(8), "x", constant(8));
    test_solve(constant(42), var("x"), "x", constant(42));
    test_solve(constant(2) * var("x"), constant(8), "x", constant(4));
    test_solve(constant(3) + constant(60) / var("zz"), constant(6), "zz", constant(20));
    test_solve(-(-constant(3)), var("x"), "x", constant(3));
    test_solve(var("x") * constant(4), var("y"), "x", var("y") / constant(4));
    test_solve(var("x") * constant(4) + constant(10), var("y"), "x", (var("y")-constant(10)) / constant(4));
}

// TODO:
// - Prioritize jobs by "badness"[not good] ("clean" rhs good? less tree depth good?)
// - Isolate each atom in turn, when find(lhs, *match_atom(rhs, the_atom)) returns false we're done
// - Take advantage of symmetry (i.e. L=R and R=L are equivalent)
// - Improve the binary_op simplification...
//
int main()
{
    extern void lex_test();
    extern void ast_test();
    lex_test();
    ast_test();
    simplify_test();
    solve_test();
    // Goal: test_solve(var("x") * constant(2), var("x") - constant(1), "x", constant(-1));
    // Subgoal:
    //test_simplify(var("x") - (var("x") * constant(2)), var("x"));
}

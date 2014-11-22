#include "source.h"
#include <stdexcept>
#include <cassert>
#include <ostream>

namespace source {

position::position(const file& source) : source_(&source), line_(1), col_(1), index_(0) {
}

position::position(const file& source, size_t line, size_t col, size_t index) : source_(&source), line_(line), col_(col), index_(index) {
    assert(line >= 1);
    assert(col >= 1);
    if (index > source.length()) {
        throw std::logic_error("position (" + std::to_string(index) + ") is out of range (" + std::to_string(source.length()) + ")");
    }
}

position position::advanced_n(size_t n) const
{
    assert(index_ + n <= source().length());
    return position(source(), line_, col_ + n, index_ + n);
}

position position::advanced_ws(char ch) const
{
    switch (ch) {
    case '\t': return position(source(), line_, col_ + (8 - col_ % 8), index_ + 1);
    case '\r': return position(source(), line_, 1, index_ + 1);
    case '\n': return position(source(), line_ + 1, 1, index_ + 1);
    default:
         assert (false);
    case '\v': case ' ': return position(source(), line_, col_ + 1, index_ + 1);
    }
}

std::ostream& operator<<(std::ostream& os, const position& pos)
{
    os << "Line " << pos.line() << ", Col " << pos.col() << ", Index " << pos.index() << " in " << pos.source().filename();
    return os;
}

} // namespace source

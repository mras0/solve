#ifndef SOLVE_SOURCE_H
#define SOLVE_SOURCE_H

#include <string>
#include <iosfwd>

namespace source {

class file {
public:
    explicit file(const std::string& filename, const std::string& contents) : filename_(filename), contents_(contents) {
    }

    const std::string& filename() const { return filename_; }
    const char* data() const { return contents_.c_str(); }
    size_t length() const { return contents_.length(); }

private:
    const std::string filename_;
    const std::string contents_;

    file(const file& s) = delete;
    file& operator=(const file& s) = delete;
};

class position {
public:
    position(const file& source);
    position(const file& source, size_t line, size_t col, size_t index);

    const file& source() const { return *source_; }
    size_t      line() const { return line_; }
    size_t      col() const  { return col_; }
    size_t      index() const { return index_; }

    const char* data() const { return source().data() + index(); }

    position    advanced_n(size_t n) const;
    position    advanced_ws(char ch) const;
private:
    const file* source_;
    size_t      line_;
    size_t      col_;
    size_t      index_;
};

std::ostream& operator<<(std::ostream& os, const position& pos);

} // namespace source

#endif

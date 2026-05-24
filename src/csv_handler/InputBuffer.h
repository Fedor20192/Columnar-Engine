#pragma once

#include <memory>
#include <string>

namespace cngn {

struct MmapRegion;

struct Parameters {
    static constexpr char kDelimiter = ',';
    static constexpr char kQuote = '"';
    static constexpr char kLinebreak = '\n';
};

class InputBuffer {
public:
    explicit InputBuffer(const std::string& filename);
    int GetChar();
    int Peek() const;
    std::string_view FindSymb(char symb) const;
    std::string_view FindDels() const;
    void UpdatePos(size_t plus);
    size_t GetSize() const;
    size_t GetPos() const;
    std::shared_ptr<MmapRegion> GetRegion() const;

private:
    std::shared_ptr<MmapRegion> region_;
    size_t buffer_pos_ = 0;
};

}  // namespace cngn

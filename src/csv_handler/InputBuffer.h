#pragma once

#include <string>
#include <memory>

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
    void CheckWindow();

    std::shared_ptr<MmapRegion> region_;
    size_t buffer_pos_ = 0, windows_bound_;
    static constexpr size_t kWindowSize = 256 * 1024 * 1024;
};

}  // namespace cngn

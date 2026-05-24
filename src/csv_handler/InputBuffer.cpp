#include "InputBuffer.h"

#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace cngn {

struct MmapRegion {
    explicit MmapRegion(const std::string& filename) {
        fd_ = open(filename.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("[CsvReader::InputBuffer]: Error opening file " + filename);
        }

        struct stat st;
        fstat(fd_, &st);
        buffer_sz_ = st.st_size;

        if (buffer_sz_ == 0) {
            return;
        }

        void* map_res = mmap(nullptr, buffer_sz_, PROT_READ, MAP_PRIVATE, fd_, 0);

        if (map_res == MAP_FAILED) {
            throw std::runtime_error("[CsvReader::InputBuffer]: Error mapping file " + filename);
        }

        buffer_ = static_cast<char*>(map_res);
        madvise(buffer_, buffer_sz_, MADV_SEQUENTIAL);
    }

    ~MmapRegion() {
        munmap(buffer_, buffer_sz_);
        close(fd_);
    }

    char* buffer_ = nullptr;
    size_t buffer_sz_ = 0;
    int fd_ = -1;
};

InputBuffer::InputBuffer(const std::string& filename) {
    region_ = std::make_shared<MmapRegion>(filename);
}

int InputBuffer::GetChar() {
    if (buffer_pos_ >= region_->buffer_sz_) {
        return EOF;
    }

    return static_cast<unsigned char>(region_->buffer_[buffer_pos_++]);
}

int InputBuffer::Peek() const {
    if (buffer_pos_ >= region_->buffer_sz_) {
        return EOF;
    }
    return static_cast<unsigned char>(region_->buffer_[buffer_pos_]);
}

std::string_view InputBuffer::FindSymb(char symb) const {
    auto pos = memchr(region_->buffer_ + buffer_pos_, symb, region_->buffer_sz_ - buffer_pos_);
    if (!pos) {
        return std::string_view(region_->buffer_ + buffer_pos_, region_->buffer_sz_ - buffer_pos_);
    }
    return std::string_view(region_->buffer_ + buffer_pos_, static_cast<const char*>(pos));
}

std::string_view InputBuffer::FindDels() const {
    std::string_view current_view(region_->buffer_ + buffer_pos_,
                                  region_->buffer_sz_ - buffer_pos_);
    static constexpr char kDels[] = {Parameters::kDelimiter, Parameters::kLinebreak,
                                     Parameters::kQuote};

    size_t found = current_view.find_first_of(std::string_view(kDels, 3));

    if (found == std::string_view::npos) {
        return std::string_view(region_->buffer_ + buffer_pos_, region_->buffer_sz_ - buffer_pos_);
    }
    return current_view.substr(0, found);
}

void InputBuffer::UpdatePos(size_t plus) {
    buffer_pos_ += plus;
}

size_t InputBuffer::GetPos() const {
    return buffer_pos_;
}

std::shared_ptr<MmapRegion> InputBuffer::GetRegion() const {
    return region_;
}

size_t InputBuffer::GetSize() const {
    return region_->buffer_sz_;
}

}  // namespace cngn
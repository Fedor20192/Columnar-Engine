#pragma once

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

namespace cngn {

class StringArena {
public:
    static constexpr size_t kBlockCap = 64 * 1024;

    std::string_view Copy(std::string_view sv) {
        if (offset_ + sv.size() > capacity_) {
            AllocateBlock(std::max(kBlockCap, sv.size()));
        }
        char* dst = current_.get() + offset_;
        std::memcpy(dst, sv.data(), sv.size());
        offset_ += sv.size();
        return {dst, sv.size()};
    }

    std::vector<std::shared_ptr<char[]>> ReleaseBlocks() {
        if (current_) {
            blocks_.push_back(std::move(current_));
        }
        capacity_ = 0;
        offset_ = 0;
        auto ans = std::move(blocks_);
        return ans;
    }

private:
    void AllocateBlock(size_t cap) {
        if (current_) {
            blocks_.push_back(std::move(current_));
        }
        current_ = std::shared_ptr<char[]>(new char[cap]);
        capacity_ = cap;
        offset_ = 0;
    }

    std::vector<std::shared_ptr<char[]>> blocks_;
    std::shared_ptr<char[]> current_;
    size_t capacity_{0};
    size_t offset_{0};
};

}  // namespace cngn

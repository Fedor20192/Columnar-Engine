#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "../kernel/Batch.h"

namespace cngn {
class Context {
public:
    Context() = default;
    explicit Context(const std::vector<std::string> &names) {
        need_columns_names_.reserve(names.size());
        for (const auto &name : names) {
            need_columns_names_.insert(name);
        }
    }

    const std::unordered_set<std::string>& GetNames() const {
        return need_columns_names_;
    }

    using Mapping = std::unordered_map<std::string, size_t>;

    void AddName(const std::string& name) {
        need_columns_names_.insert(name);
    }

    void SetMapping(Mapping&& mapping) {
        columns_mapping_ = std::move(mapping);
    }

    const Mapping& GetMapping() const {
        return columns_mapping_;
    }

private:
    std::unordered_set<std::string> need_columns_names_;
    Mapping columns_mapping_;
};

class Operator {
public:
    virtual void Open() = 0;
    virtual std::optional<Batch> Next() = 0;
    virtual void Close() = 0;

    virtual ~Operator() = default;

protected:
    Operator() = default;
    Operator(const Operator&) = delete;
    Operator(Operator&&) = delete;
    Operator& operator=(const Operator&) = delete;
    Operator& operator=(Operator&&) = delete;
};
}  // namespace cngn
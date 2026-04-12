#pragma once

#include <memory>

#include "Operator.h"

namespace cngn {
class Count : public Operator {
public:
    explicit Count(std::unique_ptr<Operator>&& next_operator)
        : next_operator_(std::move(next_operator)) {
    }
    void Open() override {
    }
    void Close() override {
    }
    std::optional<Batch> Next() override {
        if (finished_) {
            return std::nullopt;
        }
        while (auto batch = next_operator_->Next()) {
            count_ += batch.value()[0].Size();
        }
        finished_ = true;
        return Batch({Column(ArrayType<Type::Int64>(count_))}, Schema({{"count", Type::Int64}}));
    }

private:
    bool finished_{false};
    size_t count_{0};
    std::unique_ptr<Operator> next_operator_;
};
}  // namespace cngn

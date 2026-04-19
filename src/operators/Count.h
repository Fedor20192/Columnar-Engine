#pragma once

#include <memory>

#include "Operator.h"
#include "glog/logging.h"

namespace cngn {
class Count : public Operator {
public:
    explicit Count(std::unique_ptr<Operator>&& next_operator,
                   const std::shared_ptr<Context>& context)
        : Operator(context), next_operator_(std::move(next_operator)) {
    }

    void Close() override {
        next_operator_->Close();
    }

    void Open() override {
        DLOG(INFO) << "Count::Open\n";
        if (next_operator_) {
            next_operator_->Open();
        } else {
            DLOG(WARNING) << "Count operator has no son. WTF?\n";
        }
        DLOG(INFO) << "Count::Close\n";
    }

    std::optional<Batch> Next() override {
        if (finished_) {
            return std::nullopt;
        }
        while (auto batch = next_operator_->Next()) {
            if (batch.value().Empty()) {
                throw std::runtime_error("Batch is empty");
            }
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

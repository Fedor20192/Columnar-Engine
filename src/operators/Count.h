#pragma once

#include <memory>

#include "Operator.h"
#include "glog/logging.h"

namespace cngn {
class Count : public Operator {
public:
    explicit Count(std::unique_ptr<Operator>&& next_operator,
                   std::string result_column_name = "count")
        : result_column_name_(std::move(result_column_name)),
          next_operator_(std::move(next_operator)) {
    }

    void Close() override {
        next_operator_->Close();
    }

    void Open() override {
        DLOG(INFO) << "[Count]: Open\n";
        if (next_operator_) {
            next_operator_->Open();
        } else {
            DLOG(WARNING) << "[Count]: Operator has no son. WTF?\n";
        }
        DLOG(INFO) << "[Count]: Close\n";
    }

    std::optional<Batch> Next() override {
        DLOG(INFO) << "[Count]: Next\n";

        if (finished_) {
            return std::nullopt;
        }
        while (auto batch = next_operator_->Next()) {
            if (batch.value().Empty()) {
                throw std::runtime_error("[Count]: Batch is empty");
            }
            count_ += batch.value()[0].Size();
        }
        finished_ = true;

        DLOG(INFO) << "[Count]: Close\n"
                      "Count = "
                   << count_ << "\n";
        return Batch({Column(ArrayType<Type::UInt64>{count_})},
                     Schema({{result_column_name_, Type::UInt64}}));
    }

private:
    std::string result_column_name_;
    std::unique_ptr<Operator> next_operator_;
    size_t count_{0};
    bool finished_{false};
};
}  // namespace cngn

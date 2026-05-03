#pragma once

#include "Expression.h"
#include "Operator.h"

namespace cngn {
namespace operators {
class Filter : public Operator {
public:
    Filter(std::unique_ptr<Operator> child, std::shared_ptr<Expression> predicate)
        : next_operator_(std::move(child)), predicate_(std::move(predicate)) {
        if (next_operator_ == nullptr) {
            throw std::invalid_argument("[Filter]: Child cannot be nullptr");
        }
        if (predicate_ == nullptr) {
            throw std::invalid_argument("[Filter]: Predicate cannot be nullptr");
        }
    }

    void Open() override {
        next_operator_->Open();
    }

    void Close() override {
        next_operator_->Close();
    }

    std::optional<std::shared_ptr<Batch>> Next() override {
        std::shared_ptr<Batch> batch = next_operator_->Next().value_or(nullptr);
        if (!batch) {
            return std::nullopt;
        }

        auto predicate_res =
            std::get<ArrayType<Type::Bool>>(predicate_->Calculate(batch).GetData());

        size_t good_rows =
            batch->RowCount() - std::count(predicate_res.begin(), predicate_res.end(), 0);

        const auto& schema = batch->GetSchema();

        if (good_rows == 0) {
            return std::make_shared<Batch>(schema);
        }
        if (good_rows == batch->RowCount()) {
            return batch;
        }

        Batch ans(schema);

        for (size_t col_num = 0; col_num < batch->ColumnCount(); ++col_num) {
            auto construct_column = [&]<Type type>() -> Column {
                const auto& src_data = std::get<ArrayType<type>>((*batch)[col_num].GetData());
                ArrayType<type> filtered_values;
                filtered_values.reserve(good_rows);

                for (size_t row_num = 0; row_num < batch->RowCount(); ++row_num) {
                    if (!predicate_res[row_num]) {
                        continue;
                    }
                    filtered_values.push_back(src_data[row_num]);
                }
                return Column(std::move(filtered_values), (*batch)[col_num].GetOwningBuffer());
            };

            ans.AddColumn(
                DispatchOnType(schema.GetData()[col_num].column_type, std::move(construct_column)));
        }

        return std::make_shared<Batch>(std::move(ans));
    }

private:
    std::unique_ptr<Operator> next_operator_;
    std::shared_ptr<Expression> predicate_;
};
}  // namespace operators
}  // namespace cngn
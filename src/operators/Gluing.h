#pragma once
#include "Operator.h"

namespace cngn {
namespace operators {

class Gluing : public Operator {
public:
    explicit Gluing(std::vector<std::unique_ptr<Operator>> next_operators)
        : next_operators_(std::move(next_operators)) {
        if (next_operators_.empty()) {
            throw std::invalid_argument("[Gluing]: No children");
        }

        for (const auto& op : next_operators_) {
            if (op == nullptr) {
                throw std::invalid_argument("[Gluing]: Child is null");
            }
        }
    }

    void Open() override {
        for (const auto& op : next_operators_) {
            op->Open();
        }
    }

    void Close() override {
        for (const auto& op : next_operators_) {
            op->Close();
        }
        finished_ = true;
    }

    std::optional<std::shared_ptr<Batch>> Next() override {
        if (finished_) {
            return std::nullopt;
        }

        std::optional<uint64_t> need_rows;

        std::vector<Column> columns;
        std::vector<Schema::ColumnData> column_data;

        for (size_t i = 0; i < next_operators_.size(); i++) {
            auto batch = next_operators_[i]->Next().value_or(nullptr);

            if (batch == nullptr) {
                throw std::logic_error("[Gluing]: child batch is null");
            }

            if (next_operators_[i]->Next().has_value()) {
                throw std::logic_error("[Gluing]: child produced too many batches");
            }

            if (!need_rows) {
                need_rows = batch->RowCount();
            }

            if (need_rows != batch->RowCount()) {
                throw std::logic_error("[Gluing]: batch row count mismatch");
            }

            auto& schema = batch->GetSchema().GetData();

            for (size_t col_num = 0; col_num < batch->ColumnCount(); col_num++) {
                columns.push_back(std::move((*batch)[col_num]));
                column_data.push_back(schema[col_num]);
            }
        }

        finished_ = true;

        return std::make_shared<Batch>(std::move(columns), Schema(std::move(column_data)));
    }

private:
    std::vector<std::unique_ptr<Operator>> next_operators_;
    bool finished_{false};
};

}  // namespace operators
}  // namespace cngn
#pragma once

#include "Expression.h"
#include "Operator.h"

namespace cngn {
namespace operators {

struct SortKey {
    std::shared_ptr<Expression> expression;
    std::string result_column_name;
};

class Sort : public Operator {
public:
    explicit Sort(std::unique_ptr<Operator> next_operator, std::vector<SortKey> sort_meta,
                  bool is_high_order = true);

    void Open() override;

    std::optional<std::shared_ptr<Batch>> Next() override;

    void Close() override;

private:
    std::unique_ptr<Operator> next_operator_;
    std::vector<SortKey> sort_meta_;
    bool is_high_order_;
    bool finished_ = false;

    static std::shared_ptr<Batch> GlueBatches(const std::vector<std::shared_ptr<Batch>>& batches);
    static std::shared_ptr<Batch> ReorderRows(const std::shared_ptr<Batch>& batch,
                                              const std::vector<size_t>& indices);
};

class TopK : public Operator {
public:
    explicit TopK(std::unique_ptr<Operator> next_operator, std::vector<SortKey> sort_meta, size_t k,
                  bool is_high_order, size_t offset = 0);

    void Open() override;

    std::optional<std::shared_ptr<Batch>> Next() override;

    void Close() override;

private:
    std::unique_ptr<Operator> next_operator_;
    std::vector<SortKey> sort_meta_;
    size_t k_, offset_;
    bool is_high_order_;
    bool finished_ = false;
};

}  // namespace operators
}  // namespace cngn
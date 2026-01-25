#include "BatchedWriter.h"
#include "glog/logging.h"

namespace cngn {
BatchedWriter::BatchedWriter(const std::string& filename, const Schema& schema)
    : file_(filename, std::ios::binary), metadata_(schema) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Batched writer cannot open file " << filename << std::endl;
        throw std::runtime_error("Cannot open file " + filename + ".");
    }
}

void BatchedWriter::WriteBatch(const Batch& batch) {
    if (batch.Empty()) {
        DLOG(ERROR) << "Trying to write empty batch" << std::endl;
        return;
    }

    const size_t row_cnt = batch[0].Size();

    size_t now_offset{};
    for (size_t column_index = 0; column_index < batch.ColumnCount(); ++column_index) {
        for (size_t row_index = 0; row_index < row_cnt; ++row_index) {
            now_offset = WriteElem(batch[column_index][row_index]);
        }
    }
    metadata_.AddBatch(now_offset, batch.ColumnCount(), row_cnt);
}

void BatchedWriter::WriteMetadata() {
    DLOG(INFO) << "Start writing metadata" << std::endl;

    std::vector<PhysTypeVariant> serialized_metadata = metadata_.Serialize();
    for (size_t i = 0; i < serialized_metadata.size(); ++i) {
        std::visit([this](const auto& value) {
            WriteElem(ValuePad(value));
        }, serialized_metadata[i]);
    }

    DLOG(INFO) << "Finished writing metadata" << std::endl;
}

void BatchedWriter::Flush() {
    file_.flush();
}

}  // namespace cngn
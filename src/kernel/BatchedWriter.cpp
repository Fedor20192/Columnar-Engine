#include "BatchedWriter.h"

#include "glog/logging.h"

namespace cngn {
BatchedWriter::BatchedWriter(const std::string& filename, const Schema& schema)
    : file_(filename, std::ios::binary), metadata_(schema) {
    if (!file_.is_open()) {
        throw std::runtime_error("Batched writer cannot open file " + filename + ".");
    }
}

void BatchedWriter::WriteBatch(const Batch& batch) {
    if (batch.Empty()) {
        DLOG(ERROR) << "Trying to write empty batch" << '\n';
        return;
    }
    if (batch.ColumnCount() != metadata_.GetColumnsCnt()) {
        throw std::runtime_error("Bad columns count: " + std::to_string(batch.ColumnCount()) +
                                 " != " + std::to_string(metadata_.GetColumnsCnt()));
    }

    const size_t row_cnt = batch[0].Size();

    size_t now_offset{};
    for (size_t column_index = 0; column_index < batch.ColumnCount(); ++column_index) {
        now_offset = WriteElem(batch[column_index].GetData());
    }
    metadata_.AddBatch(now_offset, row_cnt);
}

void BatchedWriter::WriteMetadata() {
    DLOG(INFO) << "Start writing metadata" << '\n';

    std::vector<PhysTypeVariant> serialized_metadata = metadata_.Serialize();
    for (size_t i = 0; i < serialized_metadata.size(); ++i) {
        WriteElem(serialized_metadata[i]);
    }

    DLOG(INFO) << "Finished writing metadata" << '\n';
}

void BatchedWriter::Flush() {
    file_.flush();
}

size_t BatchedWriter::WriteElem(const PhysTypeVariant& value) {
    std::visit([this](const auto& to_print) { Write(to_print, file_); }, value);
    return file_.tellp();
}

size_t BatchedWriter::WriteElem(const ArrayTypeVariant& value) {
    std::visit([this](const auto& to_print) { Write(to_print, file_); }, value);
    return file_.tellp();
}

}  // namespace cngn
#include "CsvReader.h"

#include "glog/logging.h"

namespace cngn {

CsvReader::Chunk::Chunk() {
    Reset();
}

void CsvReader::Chunk::Add(std::string_view s) {
    fields_meta_.push_back({fields_.size(), buffer_->size(), s.size()});
    fields_.emplace_back();
    buffer_->insert(buffer_->end(), s.begin(), s.end());
}

void CsvReader::Chunk::AddSimple(std::string_view s) {
    fields_.push_back(s);
}

void CsvReader::Chunk::Prepare() {
    for (const auto& [idx, offset, size] : fields_meta_) {
        fields_[idx] = std::string_view(buffer_->data() + offset, size);
    }

    is_prepared_ = true;
}

void CsvReader::Chunk::Reset() {
    cols_cnt_ = 0;
    is_prepared_ = false;
    fields_meta_.clear();
    fields_.clear();
    buffer_ = std::make_shared<std::vector<char>>();
}

void CsvReader::Chunk::InitColumnsCnt(size_t rows_cnt) {
    cols_cnt_ = GetColsCount(rows_cnt);
}

std::string_view CsvReader::Chunk::GetField(size_t row_ind, size_t col_ind) const {
    return fields_[row_ind * cols_cnt_ + col_ind];
}

size_t CsvReader::Chunk::GetColsCount(size_t rows_cnt) const {
    if (!is_prepared_) {
        throw std::logic_error("[CsvReader::Chunk::GetColsCount]: Not prepared yet]");
    }
    if (fields_.empty()) {
        return 0;
    }
    if (rows_cnt == 0) {
        throw std::runtime_error("[CsvReader::Chunk::GetCols()]: rows_cnt is 0");
    }
    return fields_.size() / rows_cnt;
}

bool CsvReader::Chunk::Empty() const {
    if (!is_prepared_) {
        throw std::logic_error("CsvReader::Chunk::Empty(): not prepared");
    }
    return fields_.empty();
}

std::pair<std::shared_ptr<std::vector<char>>, std::shared_ptr<MmapRegion>>
CsvReader::Chunk::GetBuffer() {
    return std::make_pair(buffer_, region_);
}

void CsvReader::Chunk::SetRegion(const std::shared_ptr<MmapRegion>& region) {
    region_ = region;
}

void CsvReader::LineState::FieldState::Reset() {
    data.clear();
    direct = {};
    is_quote_open = is_quote_close = false;
}

bool CsvReader::LineState::FieldState::IsSimple() const {
    return data.empty();
}

void CsvReader::LineState::Reset() {
    need_break = has_read = false;
    is_valid = true;
    field.Reset();
}

CsvReader::CsvReader(const std::string& filename) : buffer_(filename) {
    chunk_.SetRegion(buffer_.GetRegion());
}

CsvReader::Chunk CsvReader::GetChunk() {
    chunk_.Prepare();
    Chunk ans = std::move(chunk_);
    chunk_.Reset();
    chunk_.SetRegion(buffer_.GetRegion());
    return ans;
}

bool CsvReader::ReadLine() {
    if (buffer_.Peek() == EOF) {
        return false;
    }

    state_.Reset();

    LineState::FieldState& field_state = state_.field;
    while (!state_.need_break && state_.is_valid) {
        if (!field_state.is_quote_open && buffer_.Peek() != Parameters::kQuote) {
            auto str = buffer_.FindDels();
            buffer_.UpdatePos(str.size());
            chunk_.AddSimple(str);

            int c = buffer_.GetChar();

            state_.has_read = true;

            if (c == Parameters::kLinebreak || c == EOF) {
                return true;
            }

            if (c == Parameters::kQuote) {
                state_.is_valid = false;
                DLOG(ERROR) << "[CSVReader]: Bad quote in the middle of field" << '\n';
                return false;
            }
            continue;
        }

        int c = buffer_.GetChar();
        FieldHandler(c, state_);
        state_.has_read = true;
        if (c == EOF) {
            break;
        }
    }

    if (!state_.has_read) {
        state_.is_valid = false;
    }

    if (!state_.is_valid) {
        return false;
    }

    return true;
}

void CsvReader::FieldHandler(int c, LineState& line_state) {
    LineState::FieldState& field_state = line_state.field;
    if (c == Parameters::kQuote) {
        if (field_state.data.empty() && !field_state.is_quote_open) {
            field_state.is_quote_open = true;
        } else if (buffer_.Peek() == Parameters::kQuote) {
            field_state.data += Parameters::kQuote;
            buffer_.GetChar();
        } else if (field_state.is_quote_open) {
            field_state.is_quote_close = true;
        } else {
            DLOG(ERROR) << "[CSVReader]: Bad quote in field" << '\n';
            line_state.is_valid = false;
        }
    } else if (field_state.is_quote_open && !field_state.is_quote_close) {
        field_state.data += c;
        auto str = buffer_.FindSymb(Parameters::kQuote);
        field_state.data += str;
        buffer_.UpdatePos(str.size());
    } else if (c == Parameters::kDelimiter || c == Parameters::kLinebreak || c == EOF) {
        if (field_state.IsSimple()) {
            chunk_.AddSimple(field_state.direct);
        } else {
            chunk_.Add(field_state.data);
        }
        field_state.Reset();
        if (c == Parameters::kLinebreak) {
            line_state.need_break = true;
        }
    } else if (!field_state.is_quote_open) {
        field_state.data += c;
    } else {
        DLOG(ERROR) << "[CSVReader]: Bad symbol" << '\n';
        line_state.is_valid = false;
    }
}

}  // namespace cngn
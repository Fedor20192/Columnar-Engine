#include "CsvReader.h"

#include "glog/logging.h"

namespace cngn {
CsvReader::Chunk::Chunk() {
    Reset();
}

void CsvReader::Chunk::Add(const std::string& s) {
    buffer_->insert(buffer_->end(), s.begin(), s.end());
    fields_sizes_.push_back(s.size());
}

void CsvReader::Chunk::Prepare() {
    char* buffer_ptr = buffer_->data();
    fields_.reserve(fields_sizes_.size());
    for (size_t i = 0; i < fields_sizes_.size(); ++i) {
        fields_.emplace_back(buffer_ptr, buffer_ptr + fields_sizes_[i]);
        buffer_ptr += fields_sizes_[i];
    }
    is_prepared_ = true;
}

void CsvReader::Chunk::Reset() {
    cols_cnt_ = 0;
    is_prepared_ = false;
    fields_sizes_.clear();
    fields_.clear();
    buffer_ = std::make_unique<std::vector<char>>();
}

void CsvReader::Chunk::InitColumnsCnt(size_t rows_cnt) {
    cols_cnt_ = GetColsCount(rows_cnt);
}

std::string_view CsvReader::Chunk::GetField(size_t row_ind, size_t col_ind) const {
    return fields_[row_ind * cols_cnt_ + col_ind];
}

size_t CsvReader::Chunk::GetColsCount(size_t rows_cnt) const {
    if (!is_prepared_) {
        throw std::logic_error("CsvReader::Chunk::GetCols(): not prepared");
    }
    if (fields_.empty()) {
        return 0;
    }
    if (rows_cnt == 0) {
        throw std::runtime_error("CsvReader::Chunk::GetCols(): rows_cnt is 0");
    }
    return fields_.size() / rows_cnt;
}

bool CsvReader::Chunk::Empty() const {
    if (!is_prepared_) {
        throw std::logic_error("CsvReader::Chunk::Empty(): not prepared");
    }
    return fields_.empty();
}

std::shared_ptr<std::vector<char>> CsvReader::Chunk::GetBuffer() {
    return buffer_;
}

CsvReader::InputBuffer::InputBuffer(const std::string& filename) : file_(filename) {
    if (!file_.is_open()) {
        throw std::runtime_error("Error opening file " + filename);
    }
    buffer_ = std::make_unique<char[]>(kBufCp);
    Update();
}

int CsvReader::InputBuffer::GetChar() {
    if (buffer_sz_ == buffer_pos_) {
        Update();
    }
    if (buffer_sz_ == 0) {
        return EOF;
    }

    return buffer_[buffer_pos_++];
}

int CsvReader::InputBuffer::Peek() {
    if (buffer_sz_ == buffer_pos_) {
        Update();
    }
    if (buffer_sz_ == 0) {
        return EOF;
    }
    return buffer_[buffer_pos_];
}

std::string_view CsvReader::InputBuffer::FindSymb(char symb) const {
    auto pos = memchr(buffer_.get() + buffer_pos_, symb, buffer_sz_ - buffer_pos_);
    if (!pos) {
        return std::string_view(buffer_.get() + buffer_pos_, buffer_sz_ - buffer_pos_);
    }
    return std::string_view(buffer_.get() + buffer_pos_, static_cast<const char*>(pos));
}

std::string_view CsvReader::InputBuffer::FindDels() const {
    std::string_view current_view(buffer_.get() + buffer_pos_, buffer_sz_ - buffer_pos_);
    static constexpr char kDels[] = {Parameters::kDelimiter, Parameters::kLinebreak,
                                     Parameters::kQuote};

    size_t found = current_view.find_first_of(std::string_view(kDels, 3));

    if (found == std::string_view::npos) {
        return std::string_view(buffer_.get() + buffer_pos_, buffer_sz_ - buffer_pos_);
    }
    return current_view.substr(0, found);
}

void CsvReader::InputBuffer::UpdatePos(size_t plus) {
    buffer_pos_ += plus;
    if (buffer_pos_ >= buffer_sz_) {
        Update();
    }
}

void CsvReader::InputBuffer::Update() {
    size_t offset = buffer_sz_ - buffer_pos_;

    std::memmove(buffer_.get(), buffer_.get() + buffer_pos_, offset);

    file_.read(buffer_.get() + offset, kBufCp - offset);
    buffer_sz_ = file_.gcount() + offset;
    buffer_pos_ = 0;
}

size_t CsvReader::InputBuffer::GetPos() const {
    return buffer_pos_;
}

size_t CsvReader::InputBuffer::GetSize() const {
    return buffer_sz_;
}

void CsvReader::LineState::FieldState::Reset() {
    data.clear();
    is_quote_open = is_quote_close = false;
}

void CsvReader::LineState::Reset() {
    need_break = has_read = false;
    is_valid = true;
    field.Reset();
}

CsvReader::CsvReader(const std::string& filename) : buffer_(filename) {
}

CsvReader::Chunk CsvReader::GetChunk() {
    chunk_.Prepare();
    Chunk ans = std::move(chunk_);
    chunk_.Reset();
    return ans;
}

bool CsvReader::ReadLine() {
    if (buffer_.Peek() == EOF) {
        return false;
    }

    state_.Reset();

    LineState::FieldState& field_state = state_.field;
    while (!state_.need_break && state_.is_valid) {
        if (int next = buffer_.Peek(); next != Parameters::kQuote &&
                                       next != Parameters::kDelimiter &&
                                       next != Parameters::kLinebreak && next != EOF) {
            auto str = buffer_.FindDels();
            state_.field.data += str;
            buffer_.UpdatePos(str.size());

            if ((!field_state.is_quote_open || field_state.is_quote_close) &&
                buffer_.Peek() == Parameters::kQuote) {
                state_.is_valid = false;
                DLOG(ERROR) << "[CSVReader]: Bad quote in the middle of field" << '\n';
            }
        }

        int c = buffer_.GetChar();
        FieldHandler(c, state_);
        if (c == EOF) {
            break;
        }
        state_.has_read = true;
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
        chunk_.Add(field_state.data);
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
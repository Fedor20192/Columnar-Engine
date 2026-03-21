#include "CsvReader.h"

#include "glog/logging.h"

namespace cngn {
CsvReader::Buffer::Buffer(const std::string& filename) : file_(filename) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Error opening file " << filename << '\n';
        throw std::runtime_error("Error opening file");
    }

    Update();
}

int CsvReader::Buffer::GetChar() {
    if (buffer_sz_ == buffer_pos_) {
        Update();
    }
    if (buffer_sz_ == 0) {
        return EOF;
    }

    return buffer_[buffer_pos_++];
}

int CsvReader::Buffer::Peek() {
    if (buffer_sz_ == buffer_pos_) {
        Update();
    }
    if (buffer_sz_ == 0) {
        return EOF;
    }
    return buffer_[buffer_pos_];
}

std::string_view CsvReader::Buffer::FindSymb(char symb) {
    auto pos = memchr(buffer_.data() + buffer_pos_, symb, buffer_sz_ - buffer_pos_);
    if (!pos) {
        return std::string_view(buffer_.data() + buffer_pos_, buffer_sz_ - buffer_pos_);
    }
    return std::string_view(buffer_.data() + buffer_pos_, static_cast<const char*>(pos));
}

std::string_view CsvReader::Buffer::FindDels() const {
    std::string_view current_view(buffer_.data() + buffer_pos_, buffer_sz_ - buffer_pos_);
    static constexpr char kDels[] = {Parameters::kDelimiter, Parameters::kLinebreak,
                                     Parameters::kQuote};

    size_t found = current_view.find_first_of(std::string_view(kDels, 3));

    if (found == std::string_view::npos) {
        return std::string_view(buffer_.data() + buffer_pos_, buffer_sz_ - buffer_pos_);
    }
    return current_view.substr(0, found);
}

void CsvReader::Buffer::UpdatePos(size_t plus) {
    buffer_pos_ += plus;
    if (buffer_pos_ >= buffer_sz_) {
        Update();
    }
}

void CsvReader::Buffer::Update() {
    size_t offset = buffer_sz_ - buffer_pos_;

    std::memmove(buffer_.data(), buffer_.data() + buffer_pos_, offset);

    file_.read(buffer_.data() + offset, buffer_.size() - offset);
    buffer_sz_ = file_.gcount() + offset;
    buffer_pos_ = 0;
}

size_t CsvReader::Buffer::GetPos() const {
    return buffer_pos_;
}

size_t CsvReader::Buffer::GetSize() const {
    return buffer_sz_;
}

void CsvReader::LineState::FieldState::Reset() {
    data.clear();
    is_quote_open = is_quote_close = false;
}

CsvReader::CsvReader(const std::string& filename) : buffer_(filename) {
}

std::optional<CsvReader::Row> CsvReader::ReadLine() {
    LineState state;

    LineState::FieldState& field_state = state.field;
    while (!state.need_break && state.is_valid) {
        if (int next = buffer_.Peek(); next != Parameters::kQuote &&
                                       next != Parameters::kDelimiter &&
                                       next != Parameters::kLinebreak && next != EOF) {
            auto str = buffer_.FindDels();
            state.field.data += str;
            buffer_.UpdatePos(str.size());

            if ((!field_state.is_quote_open || field_state.is_quote_close) &&
                buffer_.Peek() == Parameters::kQuote) {
                state.is_valid = false;
                DLOG(ERROR) << "Bad quote in the middle of field" << '\n';
            }
        }

        int c = buffer_.GetChar();
        FieldHandler(c, state);
        if (c == EOF) {
            break;
        }
        state.has_read = true;
    }

    if (!state.has_read) {
        state.is_valid = false;
    }

    if (!state.is_valid) {
        return std::nullopt;
    }

    return state.row;
}

void CsvReader::FieldHandler(char c, LineState& line_state) {
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
            DLOG(ERROR) << "Bad quote in field" << '\n';
            line_state.is_valid = false;
        }
    } else if (field_state.is_quote_open && !field_state.is_quote_close) {
        field_state.data += c;
        auto str = buffer_.FindSymb(Parameters::kQuote);
        field_state.data += str;
        buffer_.UpdatePos(str.size());
    } else if (c == Parameters::kDelimiter || c == Parameters::kLinebreak || c == EOF) {
        line_state.row.push_back(std::move(field_state.data));
        field_state.Reset();
        if (c == Parameters::kLinebreak) {
            line_state.need_break = true;
        }
    } else if (!field_state.is_quote_open) {
        field_state.data += c;
    } else {
        DLOG(ERROR) << "Bad symbol" << '\n';
        line_state.is_valid = false;
    }
}

std::vector<CsvReader::Row> CsvReader::ReadAllLines() {
    std::vector<Row> rows;
    while (std::optional<Row> row = ReadLine()) {
        rows.push_back(std::move(row.value()));
    }
    return rows;
}

}  // namespace cngn
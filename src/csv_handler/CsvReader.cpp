#include "CsvReader.h"

#include "glog/logging.h"

namespace cngn {
CsvReader::Buffer::Buffer(const std::string& filename): file_(filename) {
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

void CsvReader::Buffer::Update() {
    size_t offset = buffer_sz_ - buffer_pos_;

    std::memmove(buffer_.data(), buffer_.data() + buffer_pos_, offset);

    file_.read(buffer_.data() + offset, buffer_.size() - offset);
    buffer_sz_ = file_.gcount() + offset;
    buffer_pos_ = 0;
}

CsvReader::CsvReader(const std::string& filename, Parameters params)
    : parameters_(params), buffer_(filename) {
    if (parameters_.delimiter == parameters_.quote) {
        DLOG(FATAL) << "Delimiter and quote symbols are equal" << '\n';
        throw std::runtime_error("Delimiter and quote symbols are equal");
    }
    if (parameters_.delimiter == parameters_.linebreak) {
        DLOG(FATAL) << "Delimiter and linebreak symbols are equal" << '\n';
        throw std::runtime_error("Delimiter and linebreak symbols are equal");
    }
    if (parameters_.quote == parameters_.linebreak) {
        DLOG(FATAL) << "Quote and linebreak symbols are equal" << '\n';
        throw std::runtime_error("Quote and linebreak symbols are equal");
    }
}

std::optional<CsvReader::Row> CsvReader::ReadLine() {
    LineState state;

    while (!state.need_break && state.is_valid) {
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
    if (c == parameters_.quote) {
        if (field_state.data.empty() && !field_state.is_quote_open) {
            field_state.is_quote_open = true;
        } else if (buffer_.Peek() == parameters_.quote) {
            field_state.data += parameters_.quote;
            buffer_.GetChar();
        } else if (field_state.is_quote_open) {
            field_state.is_quote_close = true;
        } else {
            DLOG(ERROR) << "Bad quote in field" << '\n';
            line_state.is_valid = false;
        }
    } else if (field_state.is_quote_open && !field_state.is_quote_close) {
        field_state.data += c;
    } else if (c == parameters_.delimiter || c == parameters_.linebreak || c == EOF) {
        line_state.row.push_back(std::move(field_state.data));
        field_state = LineState::FieldState{};
        if (c == parameters_.linebreak) {
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
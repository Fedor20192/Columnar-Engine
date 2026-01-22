#include "CsvReader.h"
#include "glog/logging.h"

namespace cngn {
CsvReader::CsvReader(const std::string& filename, Parameters params)
    : parameters_(params), file_(filename) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Error opening file " << filename << std::endl;
        throw std::runtime_error("Error opening file");
    }

    if (parameters_.delimiter == parameters_.quote) {
        DLOG(FATAL) << "Delimiter and quote symbols are equal" << std::endl;
        throw std::runtime_error("Delimiter and quote symbols are equal");
    }
    if (parameters_.delimiter == parameters_.linebreak) {
        DLOG(FATAL) << "Delimiter and linebreak symbols are equal" << std::endl;
        throw std::runtime_error("Delimiter and linebreak symbols are equal");
    }
    if (parameters_.quote == parameters_.linebreak) {
        DLOG(FATAL) << "Quote and linebreak symbols are equal" << std::endl;
        throw std::runtime_error("Quote and linebreak symbols are equal");
    }
}

std::optional<CsvReader::Row> CsvReader::ReadLine() {
    LineState state;

    while (!state.need_break && state.is_valid) {
        char c = file_.get();
        if (c == EOF) {
            break;
        }
        state.has_read = true;
        FieldHandler(c, state);
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
        } else if (file_.peek() == parameters_.quote) {
            field_state.data += parameters_.quote;
            file_.get();
        } else if (field_state.is_quote_open) {
            field_state.is_quote_close = true;
        } else {
            DLOG(ERROR) << "Bad quote in field" << std::endl;
            line_state.is_valid = false;
        }
    } else if (field_state.is_quote_open && !field_state.is_quote_close) {
        field_state.data += c;
    } else if (c == parameters_.delimiter || c == parameters_.linebreak) {
        line_state.row.push_back(field_state.data);
        field_state = LineState::FieldState{};
        if (c == parameters_.linebreak) {
            line_state.need_break = true;
        }
    } else if (!field_state.is_quote_open) {
        field_state.data += c;
    } else {
        DLOG(ERROR) << "Bad symbol" << std::endl;
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
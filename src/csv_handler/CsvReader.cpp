#include "CsvReader.h"

#include <stdexcept>

cngn::CsvReader::CsvReader(const std::string& filename, Parameters params)
    : parameters_(params), file_(filename) {
    if (!file_.good()) {
        throw std::runtime_error("Error opening file");
    }

    if (parameters_.delimiter == parameters_.quote) {
        throw std::runtime_error("Delimiter and quote symbols are equal");
    }
    if (parameters_.delimiter == parameters_.linebreak) {
        throw std::runtime_error("Delimiter and linebreak symbols are equal");
    }
    if (parameters_.quote == parameters_.linebreak) {
        throw std::runtime_error("Quote and linebreak symbols are equal");
    }
}

std::optional<cngn::CsvReader::Row> cngn::CsvReader::ReadLine() {
    LineState state;

    while (file_.peek() != EOF && !state.need_break) {
        char c = file_.get();
        state.has_read = true;
        FieldHandler(c, state);
    }

    if (!state.has_read) {
        return std::nullopt;
    }

    return state.row;
}

void cngn::CsvReader::FieldHandler(char c, LineState& line_state) {
    LineState::FieldState& field_state = line_state.field;
    if (c == parameters_.quote) {
        if (field_state.data.empty()) {
            field_state.is_quote_open = true;
        } else if (file_.peek() == parameters_.quote) {
            field_state.data += parameters_.quote;
            file_.get();
        } else if (field_state.is_quote_open) {
            field_state.is_quote_close = true;
        } else {
            throw std::runtime_error("Bad quote in field");
        }
    } else if (field_state.is_quote_open && !field_state.is_quote_close) {
        field_state.data += c;
    } else if (c == parameters_.delimiter) {
        line_state.row.push_back(field_state.data);
        field_state = LineState::FieldState{};
    } else if (c == parameters_.linebreak) {
        line_state.row.push_back(field_state.data);
        line_state.need_break = true;
        field_state = LineState::FieldState{};
    } else if (!field_state.is_quote_open) {
        field_state.data += c;
    } else {
        throw std::runtime_error("Bad symbol");
    }
}

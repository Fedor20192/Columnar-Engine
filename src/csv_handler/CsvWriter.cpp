#include "CsvWriter.h"
#include "glog/logging.h"

namespace cngn {
CsvWriter::CsvWriter(const std::string& filename, Parameters parameters)
    : file_(filename), parameters_(parameters) {
    if (!file_.is_open()) {
        DLOG(ERROR) << "Error opening file " << filename << std::endl;
        throw std::runtime_error("Error opening file");
    }

    if (parameters_.delimiter == parameters_.quote) {
        DLOG(ERROR) << "Delimiter and quote symbols are equal" << std::endl;
        throw std::runtime_error("Delimiter and quote symbols are equal");
    }
    if (parameters_.delimiter == parameters_.linebreak) {
        DLOG(ERROR) << "Delimiter and linebreak symbols are equal" << std::endl;
        throw std::runtime_error("Delimiter and linebreak symbols are equal");
    }
    if (parameters_.quote == parameters_.linebreak) {
        DLOG(ERROR) << "Quote and linebreak symbols are equal" << std::endl;
        throw std::runtime_error("Quote and linebreak symbols are equal");
    }
}

void CsvWriter::WriteRow(const Row& row) {
    bool is_first = true;
    for (const auto& field : row) {
        if (!is_first) {
            file_ << parameters_.delimiter;
        }
        file_ << StringToField(field);
        is_first = false;
    }
    file_ << parameters_.linebreak;
}

std::string CsvWriter::StringToField(const std::string& str) const {
    std::string ans;
    ans.reserve(str.size() + 2);
    ans += parameters_.quote;

    for (char c : str) {
        if (c == parameters_.quote) {
            ans += parameters_.quote;
            ans += parameters_.quote;
        } else {
            ans += c;
        }
    }

    ans += parameters_.quote;
    return ans;
}

}  // namespace cngn
#include "CsvReader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "glog/logging.h"

namespace cngn {

struct CsvReader::MmapRegion {
    explicit MmapRegion(const std::string& filename) {
        fd_ = open(filename.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("[CsvReader::InputBuffer]: Error opening file " + filename);
        }

        struct stat st;
        fstat(fd_, &st);
        buffer_sz_ = st.st_size;

        if (buffer_sz_ == 0) {
            return;
        }

        void* map_res = mmap(nullptr, buffer_sz_, PROT_READ, MAP_PRIVATE, fd_, 0);

        if (map_res == MAP_FAILED) {
            throw std::runtime_error("[CsvReader::InputBuffer]: Error mapping file " + filename);
        }

        buffer_ = static_cast<char*>(map_res);
        madvise(buffer_, buffer_sz_, MADV_SEQUENTIAL);
    }

    ~MmapRegion() {
        munmap(buffer_, buffer_sz_);
        close(fd_);
    }

    char* buffer_ = nullptr;
    size_t buffer_sz_ = 0;
    int fd_ = -1;
};

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
    fields_.reserve(fields_meta_.size());

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

std::pair<std::shared_ptr<std::vector<char>>, std::shared_ptr<CsvReader::MmapRegion>>
CsvReader::Chunk::GetBuffer() {
    return std::make_pair(buffer_, region_);
}

void CsvReader::Chunk::SetRegion(const std::shared_ptr<MmapRegion>& region) {
    region_ = region;
}

CsvReader::InputBuffer::InputBuffer(const std::string& filename) {
    region_ = std::make_shared<MmapRegion>(filename);
}

int CsvReader::InputBuffer::GetChar() {
    if (buffer_pos_ >= region_->buffer_sz_) {
        return EOF;
    }

    return static_cast<unsigned char>(region_->buffer_[buffer_pos_++]);
}

int CsvReader::InputBuffer::Peek() const {
    if (buffer_pos_ >= region_->buffer_sz_) {
        return EOF;
    }
    return static_cast<unsigned char>(region_->buffer_[buffer_pos_]);
}

std::string_view CsvReader::InputBuffer::FindSymb(char symb) const {
    auto pos = memchr(region_->buffer_ + buffer_pos_, symb, region_->buffer_sz_ - buffer_pos_);
    if (!pos) {
        return std::string_view(region_->buffer_ + buffer_pos_, region_->buffer_sz_ - buffer_pos_);
    }
    return std::string_view(region_->buffer_ + buffer_pos_, static_cast<const char*>(pos));
}

std::string_view CsvReader::InputBuffer::FindDels() const {
    std::string_view current_view(region_->buffer_ + buffer_pos_,
                                  region_->buffer_sz_ - buffer_pos_);
    static constexpr char kDels[] = {Parameters::kDelimiter, Parameters::kLinebreak,
                                     Parameters::kQuote};

    size_t found = current_view.find_first_of(std::string_view(kDels, 3));

    if (found == std::string_view::npos) {
        return std::string_view(region_->buffer_ + buffer_pos_, region_->buffer_sz_ - buffer_pos_);
    }
    return current_view.substr(0, found);
}

void CsvReader::InputBuffer::UpdatePos(size_t plus) {
    buffer_pos_ += plus;
}

size_t CsvReader::InputBuffer::GetPos() const {
    return buffer_pos_;
}

std::shared_ptr<CsvReader::MmapRegion> CsvReader::InputBuffer::GetRegion() const {
    return region_;
}

size_t CsvReader::InputBuffer::GetSize() const {
    return region_->buffer_sz_;
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
        if (!field_state.is_quote_open) {
            if (int next = buffer_.Peek(); next != Parameters::kQuote &&
                                           next != Parameters::kDelimiter &&
                                           next != Parameters::kLinebreak && next != EOF) {
                auto str = buffer_.FindDels();
                buffer_.UpdatePos(str.size());
                field_state.direct = str;

                if ((!field_state.is_quote_open || field_state.is_quote_close) &&
                    buffer_.Peek() == Parameters::kQuote) {
                    state_.is_valid = false;
                    DLOG(ERROR) << "[CSVReader]: Bad quote in the middle of field" << '\n';
                }
            }
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
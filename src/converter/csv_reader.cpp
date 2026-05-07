#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <sys/types.h>

#include "assert.hpp"
#include "csv_reader.hpp"
#include "parsing.hpp"
#include "schema.hpp"

namespace columnar {

CsvReader CsvReader::open_csv(const std::string& path_to_data, const std::string& path_to_schema,
                              char delimiter) {
    std::ifstream data_file(path_to_data, std::ios::binary);
    ASS(data_file.is_open(), "file not found");  // or ask for input again?

    std::ifstream schema_file(path_to_schema, std::ios::binary);
    ASS(schema_file.is_open(), "file not found");  // or ask for input again?

    CsvReader csv_reader(std::move(data_file), std::move(schema_file), delimiter);

    return csv_reader;
}

CsvReader::CsvReader(std::ifstream&& data_file, std::ifstream&& schema_file, char delimiter)
    : data_file_(std::move(data_file)), schema_file_(std::move(schema_file)), buffer_(kReadBufSize),
      delimiter_(delimiter) {
    fill();
}

Schema CsvReader::parse_schema() {
    Schema schema;
    std::vector<char> schema_buffer(kReadSchemaSize);
    schema_file_.read(schema_buffer.data(), kReadSchemaSize);

    size_t len = static_cast<size_t>(schema_file_.gcount());
    size_t pos = 0;

    std::string name;
    std::string type;

    while (true) {
        if (pos >= len) {
            break;
        }

        const char* start = schema_buffer.data() + pos;
        size_t count = len - pos;

        const char* delimiter_ptr = static_cast<const char*>(std::memchr(start, delimiter_, count));
        if (delimiter_ptr != nullptr) {
            name = std::string(start, delimiter_ptr);
            pos += static_cast<size_t>(delimiter_ptr - start + 1);
        }

        start = schema_buffer.data() + pos;
        count = len - pos;

        const char* newline_ptr = static_cast<const char*>(std::memchr(start, '\n', count));

        if (newline_ptr != nullptr) {
            type = std::string(start, newline_ptr);
            pos += static_cast<size_t>(newline_ptr - start + 1);
        } else {
            type = std::string(start, static_cast<const char*>(schema_buffer.data() + len));
            pos += static_cast<size_t>(len - pos + 1);
            schema.add_column(name, get_data_type(type));
            break;
        }

        schema.add_column(name, get_data_type(type));
    }

    return schema;
}

bool CsvReader::parse_row(const Schema& schema, std::vector<std::string>& row) {
    bool in_quotes = false;
    char ch;

    while (true) {
        refill();

        ch = buffer_[pos_];

        if (in_quotes) {
            std::string token;
            const char* start = buffer_.data() + pos_;
            size_t count = len_ - pos_;

            const char* quote_ptr = static_cast<const char*>(std::memchr(start, '"', count));
            if (quote_ptr == nullptr) {
                std::cerr << "This should no be happenning" << "\n  at " << __FILE__ << ":"
                          << __LINE__ << "\n  in " << __func__ << std::endl;
                std::abort();
            }

            token += std::string(start, quote_ptr);
            if (increment_pos(static_cast<size_t>(quote_ptr - start + 1))) {
                row.push_back(token);
                break;
            }

            if (buffer_[pos_] == '"') {
                token += '"';
                if (increment_pos(1)) {
                    std::cerr << "Last field is missing a closing quote" << "\n  at " << __FILE__
                              << ":" << __LINE__ << "\n  in " << __func__ << std::endl;
                    std::abort();
                }
                continue;
            }
            if (buffer_[pos_] == delimiter_) {
                row.push_back(token);
                in_quotes = false;
                if (increment_pos(1)) {
                    std::cerr << "Last field is followed by a comma" << "\n  at " << __FILE__ << ":"
                              << __LINE__ << "\n  in " << __func__ << std::endl;
                    std::abort();
                }
                continue;
            }
            if (buffer_[pos_] == '\n') {
                row.push_back(token);
                in_quotes = false;
                if (increment_pos(1)) {
                    break;
                }
                ASS(row.size() == schema.get_column_count(), "invalid data format");
                return true;
            }
            std::cerr << "Invalid data format" << "\n  at " << __FILE__ << ":" << __LINE__
                      << "\n  in " << __func__ << std::endl;
            std::abort();
        }

        if (ch == '"') {
            in_quotes = true;
            if (increment_pos(1)) {
                break;
            };
            continue;
        }

        const char* start = buffer_.data() + pos_;
        size_t count = len_ - pos_;

        const char* delimiter_ptr = static_cast<const char*>(std::memchr(start, delimiter_, count));
        const char* newline_ptr = static_cast<const char*>(std::memchr(start, '\n', count));

        const char* end = nullptr;
        if (delimiter_ptr != nullptr && newline_ptr != nullptr) {
            end = (delimiter_ptr <= newline_ptr) ? delimiter_ptr : newline_ptr;
        } else if (delimiter_ptr == nullptr && newline_ptr == nullptr) {
            // only triggers if the last field
            // in the dataset is followed by EOF
            end = static_cast<const char*>(buffer_.data() + len_);
        } else {
            end = (newline_ptr == nullptr) ? delimiter_ptr : newline_ptr;
        }

        row.emplace_back(start, end);
        if (increment_pos(static_cast<size_t>(end - start + 1))) {
            break;
        }

        if (end == newline_ptr) {
            ASS(row.size() == schema.get_column_count(), "invalid data format");
            return true;
        }
    }

    ASS(row.size() == schema.get_column_count() || row.size() == 0, "invalid data format");

    return false;
}

bool CsvReader::fill() {
    data_file_.read(buffer_.data(), kReadBufSize);

    len_ = static_cast<size_t>(data_file_.gcount());
    pos_ = 0;

    bool eof = (len_ == 0);
    return eof;
}

void CsvReader::refill() {
    if ((pos_ + kBufThreshold >= len_) && !data_file_.eof()) {
        // refills when less that 1MB of data is left in the buffer
        // to avoid splitting a field across the buffers
        data_file_.seekg(-static_cast<off_t>(len_ - pos_), std::ios_base::cur);
        fill();  // cannot return true
    }
}

bool CsvReader::increment_pos(size_t n) {
    pos_ += n;
    if (pos_ >= len_) {
        if (fill()) {
            return true;
        }
    }

    return false;
}

}  // namespace columnar

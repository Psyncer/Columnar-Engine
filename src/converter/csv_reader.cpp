#include <fstream>
#include <ios>
#include <iostream>
#include <string>

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
    : data_file_(std::move(data_file)), schema_file_(std::move(schema_file)), buffer_(kBufSize),
      delimiter_(delimiter) {
    refill();
}

Schema CsvReader::parse_schema() {
    std::string line;
    Schema schema;

    while (std::getline(schema_file_, line, '\n')) {
        std::vector<std::string> tokens = split(line, delimiter_);
        std::string name = tokens[0];
        std::string type = tokens[1];

        Type data_type = get_data_type(type);
        schema.add_column(name, data_type);
    }

    return schema;
}

bool CsvReader::parse_row(const Schema& schema, std::vector<std::string>& row) {
    bool in_quotes = false;
    std::string token;

    char ch;

    while (get_char(ch)) {
        if (in_quotes) {
            if (ch == '"') {
                get_char(ch);  // instead of peeking we extract
                if (ch == '"') {
                    token += ch;
                } else {
                    in_quotes = false;
                    pos_--;  // and then restore here

                    // For
                    // =======================
                    // ...
                    // 6,"""Lucas"",199,Oslo
                    // 7,"Emi""ly",21,Tokyo
                    // ...
                    // =======================
                    // where closing " is followed by 'E'
                    // (or something other than [\n], [,] or [EOF])
                    // and tokens.size() != schema.get_column_count()
                    // does not see the problem
                    ASS(buffer_[pos_] == '\n' || buffer_[pos_] == ',', "invalid data format");
                }
            } else {
                token += ch;
            }
        } else {
            if (ch == '"' && token.empty()) {
                in_quotes = true;
            } else if (ch == delimiter_) {
                row.push_back(token);
                token.clear();
            } else if (ch == '\n') {
                row.push_back(token);
                ASS(row.size() == schema.get_column_count(), "invalid data format");
                return true;
            } else if (ch == '\r') {
                // skip to handle \r\n
            } else {
                token += ch;
            }
        }
    }

    if (!token.empty() || row.size() != 0) {
        row.push_back(token);
    }

    ASS(row.size() == schema.get_column_count() || row.size() == 0, "invalid data format");

    return false;
}

bool CsvReader::get_char(char& ch) {
    if (pos_ >= len_) {
        if (!refill()) {
            return false;
        }
    }

    ch = buffer_[pos_++];

    return true;
}

bool CsvReader::refill() {
    data_file_.read(buffer_.data(), kBufSize);

    len_ = static_cast<size_t>(data_file_.gcount());
    pos_ = 0;

    return len_ > 0;
}

}  // namespace columnar

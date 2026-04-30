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
    : data_file_(std::move(data_file)), schema_file_(std::move(schema_file)),
      delimiter_(delimiter) {
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

std::vector<std::string> CsvReader::parse_row(const Schema& schema) {
    std::vector<std::string> tokens;
    bool in_quotes = false;
    std::string current;

    char c;

    while (data_file_.get(c)) {
        if (in_quotes) {
            if (c == '"') {
                if (data_file_.peek() == '"') {
                    current += c;
                    data_file_.get(c);
                } else {
                    in_quotes = false;
                    /*
                    For
                    ...
                    6,"""Lucas"",199,Oslo
                    7,"Emi""ly",21,Tokyo
                    ...
                    where closing " is followed by 'E'
                    (or something other than [\n], [,] or [EOF])
                    and tokens.size() != schema.get_column_count()
                    does not see the problem
                    */
                    ASS(data_file_.peek() == '\n' || data_file_.peek() == ',' || !data_file_.eof(),
                        "invalid data format");
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"' && current.empty()) {
                in_quotes = true;
            } else if (c == delimiter_) {
                tokens.push_back(current);
                current.clear();
            } else if (c == '\n') {
                tokens.push_back(current);
                ASS(tokens.size() == schema.get_column_count(), "invalid data format");
                return tokens;
            } else if (c == '\r') {
                // skip to handle \r\n
            } else {
                current += c;
            }
        }
    }

    if (!current.empty() || tokens.size() != 0) {
        tokens.push_back(current);
    }

    ASS(tokens.size() == schema.get_column_count() || tokens.size() == 0, "invalid data format");

    return tokens;
}

bool CsvReader::eof() const {
    return data_file_.eof();
}

}  // namespace columnar

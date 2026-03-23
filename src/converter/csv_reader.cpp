#include "csv_reader.hpp"
#include "parse_error.hpp"
#include "parsing.hpp"
#include "schema.hpp"

#include <expected>
#include <fstream>
#include <ios>
#include <string>

namespace columnar {

Expected<CsvReader> CsvReader::open_csv(const std::string& path_to_data,
                                        const std::string& path_to_schema, char delimiter) {
    std::ifstream data_file(path_to_data, std::ios::binary);
    if (!data_file.is_open()) {
        return std::unexpected(parse_error::file_not_found);
    }

    std::ifstream schema_file(path_to_schema, std::ios::binary);
    if (!schema_file.is_open()) {
        return std::unexpected(parse_error::file_not_found);
    }

    CsvReader csv_reader(std::move(data_file), std::move(schema_file), delimiter);

    return csv_reader;
}

CsvReader::CsvReader(std::ifstream&& data_file, std::ifstream&& schema_file, char delimiter)
    : data_file_(std::move(data_file)), schema_file_(std::move(schema_file)),
      delimiter_(delimiter) {
}

Expected<Schema> CsvReader::parse_schema() {
    std::string line;
    Schema schema;

    while (std::getline(schema_file_, line, '\n')) {
        auto tokens = split(line, delimiter_);
        if (!tokens.has_value()) {
            return std::unexpected(tokens.error());
        }

        std::string name = (*tokens)[0];
        std::string type = (*tokens)[1];

        auto data_type = get_data_type(type);
        if (!data_type.has_value()) {
            return std::unexpected(data_type.error());
        }

        schema.add_column(name, *data_type);
    }

    return schema;
}

Expected<std::vector<std::string>> CsvReader::parse_row(const Schema& schema) {
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
                    if (data_file_.peek() != '\n' && data_file_.peek() != ',' &&
                        !data_file_.eof()) {
                        return std::unexpected(parse_error::invalid_data_format);
                    }
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
                if (tokens.size() != schema.get_column_count()) {
                    return std::unexpected(parse_error::invalid_data_format);
                }
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

    if (tokens.size() != schema.get_column_count() && tokens.size() != 0) {
        return std::unexpected(parse_error::invalid_data_format);
    }

    return tokens;
}

bool CsvReader::eof() const {
    return data_file_.eof();
}

}  // namespace columnar

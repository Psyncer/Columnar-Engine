#pragma once

#include "parse_error.hpp"
#include "schema.hpp"

#include <expected>
#include <fstream>
#include <string>

namespace columnar {

class CsvReader {
private:
    std::ifstream data_file_{};
    std::ifstream schema_file_{};
    char delimiter_ = ',';

public:
    static Expected<CsvReader> open_csv(const std::string& path_to_data,
                                        const std::string& path_to_schema, char delimiter = ',');

    Expected<Schema> parse_schema();

    Expected<std::vector<std::string>> parse_row(const Schema& schema);

    bool eof() const;

private:
    CsvReader(std::ifstream&& data_file, std::ifstream&& schema_file, char delimiter = ',');
};

}  // namespace columnar

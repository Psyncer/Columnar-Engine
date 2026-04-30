#pragma once

#include <fstream>
#include <string>

#include "schema.hpp"

namespace columnar {

class CsvReader {
private:
    std::ifstream data_file_;
    std::ifstream schema_file_;
    char delimiter_ = ',';

public:
    static CsvReader open_csv(const std::string& path_to_data, const std::string& path_to_schema,
                              char delimiter = ',');

    Schema parse_schema();  // throws

    std::vector<std::string> parse_row(const Schema& schema);  // throws

    bool eof() const;

private:
    CsvReader(std::ifstream&& data_file, std::ifstream&& schema_file, char delimiter = ',');
};

}  // namespace columnar

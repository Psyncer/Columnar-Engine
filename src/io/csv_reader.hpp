#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "src/storage/schema.hpp"

namespace columnar {
    
class CsvReader {
private:
    std::ifstream data_file_;
    std::ifstream schema_file_;
    std::vector<char> buffer_;
    size_t len_ = 0;
    size_t pos_ = 0;
    char delimiter_ = ',';

public:
    static CsvReader open_csv(const std::string& path_to_data, const std::string& path_to_schema,
                              char delimiter = ',');

    Schema parse_schema();  // throws

    bool parse_row(const Schema& schema, std::vector<std::string>& row);  // throws

private:
    CsvReader(std::ifstream&& data_file, std::ifstream&& schema_file, char delimiter);

    bool fill();  // throws

    void refill();  // throws

    bool increment_pos(size_t n);  // throws
};

}  // namespace columnar

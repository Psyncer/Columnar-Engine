#pragma once

#include "batch.hpp"
#include "data_type.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

// #include <expected>
#include <fstream>
#include <string>

namespace columnar {

class CsvReader {
public:
    static CsvReader open_csv_to_read(const std::string& path, char delimiter = ',');

    static DataType get_data_type(const std::string& type);

    static Schema parse_schema_file(const std::string& path, char delimiter = ',');

    bool fill_batch(Batch& batch);

private:
    std::ifstream file_;
    char delimiter_;
    bool eof_;

    CsvReader(std::ifstream file, char delimiter = ',');
};

}  // namespace columnar

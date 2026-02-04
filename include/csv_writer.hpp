#pragma once

#include "batch.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

// #include <expected>
#include <fstream>
#include <string>

namespace columnar {

class CsvWriter {
public:
    static CsvWriter open_csv_to_write(const std::string& path, const Schema& schema);

    void write_batch(const Batch& batch);

    void write_schema(const std::string& path);

private:
    std::ofstream file_;
    const Schema& schema_;

    CsvWriter(std::ofstream file, const Schema& schema);
};

}  // namespace columnar

#pragma once

#include "batch.hpp"
#include "parse_error.hpp"
#include "schema.hpp"

#include <expected>
#include <fstream>
#include <string>

namespace columnar {

class CsvWriter {
private:
    std::ofstream file_{};
    const Schema& schema_{};

public:
    static Expected<CsvWriter> open_csv_to_write(const std::string& path, const Schema& schema);

    void write_batch(const Batch& batch);

    void write_schema(const std::string& path);

private:
    CsvWriter(std::ofstream&& file, const Schema& schema);

    static std::string escape_csv_field(const std::string& field);
};

}  // namespace columnar

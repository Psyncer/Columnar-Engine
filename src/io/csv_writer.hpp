#pragma once

#include <string>

#include "src/storage/batch.hpp"

namespace columnar {

class CsvWriter {
private:
    // std::ofstream file_{};
    // const Schema& schema_{};

public:
    // static CsvWriter open_csv_to_write(const std::string& path, const Schema& schema);

    static void write_batch(const Batch& batch);  // throws

    // void write_schema(const std::string& path);

    static std::string convert_to_date(int32_t days);

    static std::string convert_to_timestamp(int64_t seconds);

private:
    // CsvWriter(std::ofstream&& file, const Schema& schema);

    // static std::string escape_csv_field(const std::string& field);
};

}  // namespace columnar

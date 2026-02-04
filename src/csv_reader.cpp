#include "csv_reader.hpp"
#include "batch.hpp"
#include "csv_utils.hpp"
#include "data_type.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

// #include <expected>
#include <fstream>
#include <string>

namespace columnar {

CsvReader CsvReader::open_csv_to_read(const std::string& path, char delimiter) {
    std::ifstream file(path, std::ios::binary);
    CsvReader reader(std::move(file), delimiter);

    return reader;
}

DataType CsvReader::get_data_type(const std::string& type) {
    if (type == "int64") {
        return DataType::int64;
    }

    return DataType::string;
}

Schema CsvReader::parse_schema_file(const std::string& path, char delimiter) {
    std::ifstream file(path, std::ios::binary);
    std::string line;
    Schema schema;

    while (std::getline(file, line, '\n')) {
        auto tokens = split(line, delimiter);
        std::string name = (tokens)[0];
        std::string type = (tokens)[1];
        auto data_type = get_data_type(type);
        schema.add_column(name, data_type);
    }

    return schema;
}

bool CsvReader::fill_batch(Batch& batch) {
    if (eof_) {
        return false;
    }

    std::string line;

    while (!batch.is_full() && std::getline(file_, line, '\n')) {
        if (line == "") {
            continue;
        }

        auto tokens = split(line, delimiter_);
        batch.add_row(tokens);
    }

    if (file_.eof()) {
        eof_ = true;
        return false;
    }

    return true;
}

CsvReader::CsvReader(std::ifstream file, char delimiter)
    : file_(std::move(file)), delimiter_(delimiter), eof_(false) {
}

}  // namespace columnar

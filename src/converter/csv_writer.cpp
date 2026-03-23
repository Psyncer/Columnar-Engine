#include "csv_writer.hpp"
#include "batch.hpp"
#include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
#include <expected>
#include <fstream>
#include <string>

namespace columnar {

Expected<CsvWriter> CsvWriter::open_csv_to_write(const std::string& path, const Schema& schema) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return std::unexpected(parse_error::file_not_found);
    }

    CsvWriter writer(std::move(file), schema);

    return writer;
}

CsvWriter::CsvWriter(std::ofstream&& file, const Schema& schema)
    : file_(std::move(file)), schema_(schema) {
}

void CsvWriter::write_batch(const Batch& batch) {
    for (size_t row = 0; row < batch.get_row_count(); ++row) {
        for (size_t col = 0; col < schema_.get_column_count(); ++col) {
            auto column_data = batch.get_column(col);
            DataType type = schema_.get_column(col)->type_;

            if (type == DataType::int64) {
                const auto& column = column_data->get<int64_t>();
                file_ << (*column)[row];
            } else if (type == DataType::string) {
                const auto& column = column_data->get<std::string>();
                file_ << escape_csv_field((*column)[row]);
            }

            if (col < schema_.get_column_count() - 1) {
                file_ << ',';
            }
        }
        file_ << '\n';
    }
}

void CsvWriter::write_schema(const std::string& path) {
    std::ofstream file(path);
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        file << schema_.get_column(i)->name_ << "," << schema_.get_column(i)->type_ << '\n';
    }
}

std::string CsvWriter::escape_csv_field(const std::string& field) {
    bool needs_quotes = field.find(',') != std::string::npos ||
                        field.find('"') != std::string::npos ||
                        field.find('\n') != std::string::npos;

    if (!needs_quotes) {
        return field;
    }

    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += '"';

    return result;
}

}  // namespace columnar

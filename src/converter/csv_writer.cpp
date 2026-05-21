#include <cstdint>
#include <expected>
#include <string>

#include "batch.hpp"
#include "csv_writer.hpp"

namespace columnar {

// CsvWriter CsvWriter::open_csv_to_write(const std::string& path, const Schema& schema) {
//     std::ofstream file(path);
//     ASS(file.is_open(), "file not found");

//     CsvWriter writer(std::move(file), schema);

//     return writer;
// }

// CsvWriter::CsvWriter(std::ofstream&& file, const Schema& schema)
//     : file_(std::move(file)), schema_(schema) {
// }

void CsvWriter::write_batch(const Batch& batch) {
    // breaks if row_count is greater than
    // the ACTUAL number of rows in a batch,
    // need to assert somewhere

    size_t row_count = batch.row_count_;
    size_t column_count = batch.column_count_;

    for (size_t row = 0; row < row_count; ++row) {
        for (size_t col = 0; col < column_count; ++col) {
            const Column& column = batch.columns_[col];
            Type type = column.type();

            switch (type) {
            case Type::Int16: {
                auto value = column.get_value<int16_t>(row);
                std::cout << value << ' ';
                break;
            }
            case Type::Int32: {
                auto value = column.get_value<int32_t>(row);
                std::cout << value << ' ';
                break;
            }
            case Type::Int64: {
                auto value = column.get_value<int64_t>(row);
                std::cout << value << ' ';
                break;
            }
            case Type::String: {
                auto value = column.get_string(row);
                std::cout << value << ' ';
                break;
            }
            case Type::Date: {
                auto value = column.get_value<int32_t>(row);
                std::cout << convert_to_date(value) << ' ';
                break;
            }
            case Type::Timestamp: {
                auto value = column.get_value<int64_t>(row);
                std::cout << convert_to_timestamp(value) << ' ';
                break;
            }
            }
        }

        std::cout << '\n';
    }
}

// void CsvWriter::write_schema(const std::string& path) {
//     std::ofstream file(path);
//     for (size_t i = 0; i < schema_.get_column_count(); ++i) {
//         file << schema_.get_column_name(i) << "," << schema_.get_column_type(i) << '\n';
//     }
// }

// std::string CsvWriter::escape_csv_field(const std::string& field) {
//     bool needs_quotes = field.find(',') != std::string::npos ||
//                         field.find('"') != std::string::npos ||
//                         field.find('\n') != std::string::npos;

//     if (!needs_quotes) {
//         return field;
//     }

//     std::string result = "\"";
//     for (char c : field) {
//         if (c == '"') {
//             result += "\"\"";
//         } else {
//             result += c;
//         }
//     }
//     result += '"';

//     return result;
// }

// FIX
std::string CsvWriter::convert_to_date(int32_t days) {
    const int z = static_cast<int>(days) + 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const int doe = z - era * 146097;
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = yoe + era * 400;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp = (5 * doy + 2) / 153;
    const int d = doy - (153 * mp + 2) / 5 + 1;
    const int m = mp + (mp < 10 ? 3 : -9);
    y += static_cast<int>(m <= 2);

    char buf[11];
    buf[0] = static_cast<char>('0' + (y / 1000));
    buf[1] = static_cast<char>('0' + (y / 100 % 10));
    buf[2] = static_cast<char>('0' + (y / 10 % 10));
    buf[3] = static_cast<char>('0' + (y % 10));
    buf[4] = '-';
    buf[5] = static_cast<char>('0' + (m / 10));
    buf[6] = static_cast<char>('0' + (m % 10));
    buf[7] = '-';
    buf[8] = static_cast<char>('0' + (d / 10));
    buf[9] = static_cast<char>('0' + (d % 10));
    buf[10] = '\0';

    return std::string(static_cast<const char*>(buf), 10);
}

// FIX
std::string CsvWriter::convert_to_timestamp(int64_t seconds) {
    int32_t days = static_cast<int32_t>(seconds / 86400);
    int64_t time = seconds % 86400;
    if (time < 0) {
        days--;
        time += 86400;
    }
    int h = static_cast<int>(time / 3600);
    int mi = static_cast<int>((time % 3600) / 60);
    int s = static_cast<int>(time % 60);

    std::string result = convert_to_date(days);
    result.resize(19);
    result[10] = ' ';
    result[11] = static_cast<char>('0' + (h / 10));
    result[12] = static_cast<char>('0' + (h % 10));
    result[13] = ':';
    result[14] = static_cast<char>('0' + (mi / 10));
    result[15] = static_cast<char>('0' + (mi % 10));
    result[16] = ':';
    result[17] = static_cast<char>('0' + (s / 10));
    result[18] = static_cast<char>('0' + (s % 10));

    return result;
}

}  // namespace columnar

#include <iostream>

#include "assert.hpp"
#include "batch.hpp"
#include "columnar_reader.hpp"
#include "columnar_writer.hpp"
#include "csv_reader.hpp"
#include "csv_writer.hpp"

void test_writing(const std::string& csv_schema, const std::string& csv_data,
                  const std::string& output_file) {

    columnar::CsvReader reader = columnar::CsvReader::open_csv(csv_data, csv_schema);

    columnar::Schema schema = reader.parse_schema();

    columnar::ColumnarWriter writer = columnar::ColumnarWriter::open_output(output_file, schema);

    columnar::Batch batch(schema, 5'000);

    std::vector<std::string> row;
    row.reserve(schema.get_column_count());
    while (reader.parse_row(schema, row)) {
        if (row.empty()) {
            break;
        }
        batch.add_row(row);

        if (batch.is_full()) {
            writer.write_batch(batch);
            batch.clear();
        }

        row.clear();
    }

    if (!row.empty()) {
        batch.add_row(row);
    }

    if (batch.get_row_count() != 0) {
        writer.write_batch(batch);
        batch.clear();
    }

    writer.write_metadata();
}

void test_reading(const std::string& reconstructed_schema, const std::string& reconstructed_data,
                  const std::string& output_file) {

    columnar::ColumnarReader reader = columnar::ColumnarReader::open_output(output_file);

    const columnar::Schema& schema = reader.schema();

    columnar::CsvWriter schema_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_schema, schema);

    schema_writer.write_schema(reconstructed_schema);

    columnar::CsvWriter data_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_data, schema);

    columnar::Batch batch(schema, 5'000);

    while (true) {
        bool has_more = reader.fill_batch(batch);

        if (batch.get_row_count() > 0) {
            data_writer.write_batch(batch);
            batch.clear();
        }

        if (!has_more) {
            break;
        }
    }

    if (batch.get_row_count() > 0) {
        data_writer.write_batch(batch);
    }

    batch.clear();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <schema.csv>" << " <data.csv>" << std::endl;
        return 1;
    }

    std::string csv_schema = argv[1];
    std::string csv_data = argv[2];
    std::string output_file = "/home/mike/Columnar-Engine/tests/output.columnar";

    std::cout << "Converting CSV to columnar file..." << std::endl;

    try {
        test_writing(csv_schema, csv_data, output_file);
    } catch (...) {
        ASS(false, "need to log STL exceptions");
    }

    std::string reconstructed_schema = "/home/mike/Columnar-Engine/tests/schema_reconstructed.csv";
    std::string reconstructed_data = "/home/mike/Columnar-Engine/tests/data_reconstructed.csv";

    std::cout << "Converting columnar file back to CSV..." << std::endl;

    try {
        test_reading(reconstructed_schema, reconstructed_data, output_file);
    } catch (...) {
        ASS(false, "need to log STL exceptions");
    }

    return 0;
}

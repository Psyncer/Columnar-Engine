#include "batch.hpp"
#include "columnar_reader.hpp"
#include "columnar_writer.hpp"
#include "csv_reader.hpp"
#include "csv_utils.hpp"
#include "csv_writer.hpp"

#include <iostream>

void test_writing(const std::string& csv_schema, const std::string& csv_data,
                 const std::string& output_file) {

    auto schema = columnar::CsvReader::parse_schema_file(csv_schema);

    auto reader = columnar::CsvReader::open_csv_to_read(csv_data);

    auto writer = columnar::ColumnarWriter::open_columnar_to_write(output_file, schema);

    columnar::Batch batch(schema, 3);

    while (true) {
        auto read = reader.fill_batch(batch);
        if (batch.get_row_count() > 0) {
            writer.write_batch(batch);
        }
        if (!read) {
            break;
        }
        batch.clear();
    }

    writer.write_metadata();
}

void test_reading(const std::string& reconstructed_schema, const std::string& reconstructed_data,
                 const std::string& output_file) {

    auto reader = columnar::ColumnarReader::open_columnar_to_read(output_file);

    const columnar::Schema& schema = reader.schema();

    columnar::CsvWriter schema_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_schema, schema);

    schema_writer.write_schema(reconstructed_schema);

    columnar::CsvWriter data_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_data, schema);

    columnar::Batch batch(schema, 3);

    while (true) {
        bool has_more = reader.fill_batch(batch);

        if (batch.get_row_count() > 0) {
            data_writer.write_batch(batch);
        }

        if (!has_more) {
            break;
        }

        batch.clear();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <schema.csv>" << " <data.csv>" << std::endl;
        return 1;
    }

    std::string csv_schema = argv[1];
    std::string csv_data = argv[2];
    std::string output_file = "/home/mike/Columnar-Engine/tests/output.columnar";

    test_writing(csv_schema, csv_data, output_file);

    std::string reconstructed_schema = "/home/mike/Columnar-Engine/tests/schema_reconstructed.csv";
    std::string reconstructed_data = "/home/mike/Columnar-Engine/tests/data_reconstructed.csv";

    test_reading(reconstructed_schema, reconstructed_data, output_file);

    return 0;
}

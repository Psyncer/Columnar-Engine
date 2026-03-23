#include "batch.hpp"
#include "columnar_reader.hpp"
#include "columnar_writer.hpp"
#include "csv_reader.hpp"
#include "csv_writer.hpp"
#include "parse_error.hpp"

#include <expected>
#include <iostream>

columnar::Expected<void> test_writing(const std::string& csv_schema, const std::string& csv_data,
                                      const std::string& output_file) {

    auto reader = columnar::CsvReader::open_csv(csv_data, csv_schema);
    if (!reader.has_value()) {
        return std::unexpected(reader.error());
    }

    auto schema = reader->parse_schema();
    if (!schema.has_value()) {
        return std::unexpected(schema.error());
    }

    auto writer = columnar::ColumnarWriter::open_output(output_file, *schema);

    columnar::Batch batch(*schema, 3);

    while (true) {
        if (reader->eof()) {
            break;
        }

        auto read = reader->parse_row(*schema);
        if (!read.has_value()) {
            return std::unexpected<columnar::parse_error>(read.error());
        }

        if (!(read->size() == 0)) {
            auto res = batch.add_row(*read);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }
        }

        if (batch.is_full()) {
            auto res = writer->write_batch(batch);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }

            batch.clear();
        }
    }

    if (batch.get_row_count() != 0) {
        auto res = writer->write_batch(batch);
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }

        batch.clear();
    }

    auto res = writer->write_metadata();
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    return {};
}

columnar::Expected<void> test_reading(const std::string& reconstructed_schema, const std::string& reconstructed_data,
                  const std::string& output_file) {

    auto reader = columnar::ColumnarReader::open_output(output_file);
    if (!reader.has_value()) {
        return std::unexpected(reader.error());
    }

    const columnar::Schema& schema = reader->schema();
  
    auto schema_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_schema, schema);
    if (!schema_writer.has_value()) {
        return std::unexpected(schema_writer.error());
    }

    schema_writer->write_schema(reconstructed_schema);

    auto data_writer =
        columnar::CsvWriter::open_csv_to_write(reconstructed_data, schema);
    if (!data_writer.has_value()) {
        return std::unexpected(data_writer.error());
    }

    columnar::Batch batch(schema, 3);

    while (true) {
        auto has_more = reader->fill_batch(batch);
        if (!has_more.has_value()) {
            return std::unexpected(has_more.error());
        }

        if (batch.get_row_count() > 0) {
            data_writer->write_batch(batch);
        }

        if (!(*has_more)) {
            break;
        }

        batch.clear();
    }

    return {};
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <schema.csv>" << " <data.csv>" << std::endl;
        return 1;
    }
    
    // review the namings everywhere

    std::string csv_schema = argv[1];
    std::string csv_data = argv[2];
    std::string output_file = "/home/mike/Columnar-Engine/tests/output.columnar";

    auto res1 = test_writing(csv_schema, csv_data, output_file);
    if (!res1.has_value()) {
        // output error into a logger
        std::cerr << "Error: " << res1.error() << std::endl;
        return 1;
    }

    std::string reconstructed_schema = "/home/mike/Columnar-Engine/tests/schema_reconstructed.csv";
    std::string reconstructed_data = "/home/mike/Columnar-Engine/tests/data_reconstructed.csv";

    auto res2 = test_reading(reconstructed_schema, reconstructed_data, output_file);
    if (!res2.has_value()) {
        // output error into a logger
        std::cerr << "Error: " << res2.error() << std::endl;
        return 1;
    }

    return 0;
}

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "aggregate.hpp"
#include "batch.hpp"
#include "columnar_writer.hpp"
#include "conversion_batch.hpp"
#include "csv_reader.hpp"
#include "csv_writer.hpp"
#include "expression.hpp"
#include "operator.hpp"

void test_writing(const std::string& csv_schema, const std::string& csv_data,
                  const std::string& output_file) {

    columnar::CsvReader reader = columnar::CsvReader::open_csv(csv_data, csv_schema);

    columnar::Schema schema = reader.parse_schema();

    columnar::ColumnarWriter writer = columnar::ColumnarWriter::open_output(output_file, schema);

    columnar::ConversionBatch batch(schema);

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

// void test_reading(const std::string& reconstructed_schema, const std::string& reconstructed_data,
//                   const std::string& output_file) {

//     columnar::ColumnarReader reader = columnar::ColumnarReader::open_output(output_file);

//     const columnar::Schema& schema = reader.schema();

//     columnar::CsvWriter schema_writer =
//         columnar::CsvWriter::open_csv_to_write(reconstructed_schema, schema);

//     schema_writer.write_schema(reconstructed_schema);

//     columnar::CsvWriter data_writer =
//         columnar::CsvWriter::open_csv_to_write(reconstructed_data, schema);

//     columnar::ConversionBatch batch(schema);

//     while (true) {
//         bool has_more = reader.fill_batch(batch);

//         if (batch.get_row_count() > 0) {
//             data_writer.write_batch(batch);
//             batch.clear();
//         }

//         if (!has_more) {
//             break;
//         }
//     }

//     if (batch.get_row_count() > 0) {
//         data_writer.write_batch(batch);
//     }

//     batch.clear();
// }

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <schema.csv>" << " <data.csv>" << std::endl;
        return 1;
    }

    std::string csv_schema = argv[1];
    std::string csv_data = argv[2];
    std::string output_file = "/home/mike/Columnar-Engine/tests/output.columnar";

    // std::cout << "\nConverting CSV to columnar file..." << std::endl;

    // try {
    //     test_writing(csv_schema, csv_data, output_file);
    // } catch (...) {
    //     std::cerr << "Need to log STL exceptions" << "\n  at " << __FILE__ << ":" << __LINE__
    //               << "\n  in " << __func__ << std::endl;
    //     std::abort();
    // }

    // ==============
    // Query 1
    // ==============
    {
        std::cout << "Query 1" << std::endl;

        auto scan =
            std::make_unique<columnar::ScanOperator>(output_file, std::vector<std::string>{});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::Count, "*"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 2
    // ==============
    {
        std::cout << "\nQuery 2" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(
            output_file, std::vector<std::string>{"AdvEngineID"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::Count, "*"}};

        auto expr = std::make_unique<columnar::NotEqual>(
            std::make_unique<columnar::ColumnRef>("AdvEngineID"),
            std::make_unique<columnar::Literal>(0));

        auto filter = std::make_unique<columnar::FilterOperator>(std::move(scan), std::move(expr));

        auto agg =
            std::make_unique<columnar::GlobalAggOperator>(std::move(filter), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 3
    // ==============
    {
        std::cout << "\nQuery 3" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(
            output_file, std::vector<std::string>{"AdvEngineID", "ResolutionWidth"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::Sum, "AdvEngineID"},
                                             {columnar::AggType::Count, "*"},
                                             {columnar::AggType::Avg, "ResolutionWidth"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 4
    // ==============
    {
        std::cout << "\nQuery 4" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(output_file,
                                                             std::vector<std::string>{"UserID"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::Avg, "UserID"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 5
    // ==============
    {
        std::cout << "\nQuery 5" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(output_file,
                                                             std::vector<std::string>{"UserID"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::CountDistinct, "UserID"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 6
    // ==============
    {
        std::cout << "\nQuery 6" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(
            output_file, std::vector<std::string>{"SearchPhrase"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::StrCountDistinct, "SearchPhrase"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 7
    // ==============
    {
        std::cout << "\nQuery 7" << std::endl;

        auto scan = std::make_unique<columnar::ScanOperator>(output_file,
                                                             std::vector<std::string>{"EventDate"});

        std::vector<columnar::AggSpec> specs{{columnar::AggType::Min, "EventDate"},
                                             {columnar::AggType::Max, "EventDate"}};

        auto agg = std::make_unique<columnar::GlobalAggOperator>(std::move(scan), std::move(specs));

        auto start = std::chrono::steady_clock::now();
        columnar::Batch* output_batch = agg->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << columnar::CsvWriter::convert_to_date(
                             static_cast<int32_t>(output_batch->columns_[i].get_value<int64_t>(0)))
                      << ',';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    return 0;
}

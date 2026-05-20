#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "aggregate.hpp"
#include "batch.hpp"
#include "columnar_writer.hpp"
#include "conversion_batch.hpp"
#include "csv_reader.hpp"
#include "expression.hpp"
#include "operator.hpp"

using columnar::AggSpec;
using columnar::AggType;
using columnar::Batch;
using columnar::ColumnarWriter;
using columnar::ColumnRef;
using columnar::ConversionBatch;
using columnar::CsvReader;
using columnar::FilterOperator;
using columnar::GlobalAggOperator;
using columnar::Literal;
using columnar::NotEqual;
using columnar::ScanOperator;
using columnar::Schema;

void convert(const std::string& csv_schema, const std::string& csv_data,
             const std::string& output_file) {

    CsvReader reader = CsvReader::open_csv(csv_data, csv_schema);

    Schema schema = reader.parse_schema();

    ColumnarWriter writer = ColumnarWriter::open_output(output_file, schema);

    ConversionBatch batch(schema);

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

//     ColumnarReader reader = ColumnarReader::open_output(output_file);

//     const Schema& schema = reader.schema();

//     CsvWriter schema_writer =
//         CsvWriter::open_csv_to_write(reconstructed_schema, schema);

//     schema_writer.write_schema(reconstructed_schema);

//     CsvWriter data_writer =
//         CsvWriter::open_csv_to_write(reconstructed_data, schema);

//     ConversionBatch batch(schema);

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

template <typename... Ts>
std::vector<AggSpec> make_aggs(Ts&&... ts) {
    std::vector<AggSpec> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

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
    //     convert(csv_schema, csv_data, output_file);
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

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{}),
            make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 2
    // ==============
    {
        std::cout << "\nQuery 2" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<FilterOperator>(
                std::make_unique<ScanOperator>(output_file,
                                               std::vector<std::string>{"AdvEngineID"}),
                std::make_unique<NotEqual>(std::make_unique<ColumnRef>("AdvEngineID"),
                                           std::make_unique<Literal>(0))),
            make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 3
    // ==============
    {
        std::cout << "\nQuery 3" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(
                output_file, std::vector<std::string>{"AdvEngineID", "ResolutionWidth"}),
            make_aggs(
                AggSpec(AggType::Sum, "AdvEngineID", std::make_unique<ColumnRef>("AdvEngineID")),
                AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*")),
                AggSpec(AggType::Avg, "ResolutionWidth",
                        std::make_unique<ColumnRef>("ResolutionWidth"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        for (size_t i = 0; i < output_batch->column_count_; ++i) {
            std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
        }

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // // ==============
    // // Query 4
    // // ==============
    // {
    //     std::cout << "\nQuery 4" << std::endl;

    //     auto scan = std::make_unique<ScanOperator>(output_file,
    //     std::vector<std::string>{"UserID"});

    //     std::vector<AggSpec> specs{{AggType::Avg, "UserID"}};

    //     auto agg = std::make_unique<GlobalAggOperator>(std::move(scan), std::move(specs));

    //     auto start = std::chrono::steady_clock::now();
    //     Batch* output_batch = agg->next();
    //     auto end = std::chrono::steady_clock::now();

    //     for (size_t i = 0; i < output_batch->column_count_; ++i) {
    //         std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
    //     }

    //     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "\nTime: " << ms << " ms" << std::endl;
    // }

    // // ==============
    // // Query 5
    // // ==============
    // {
    //     std::cout << "\nQuery 5" << std::endl;

    //     auto scan = std::make_unique<ScanOperator>(output_file,
    //     std::vector<std::string>{"UserID"});

    //     std::vector<AggSpec> specs{{AggType::CountDistinct, "UserID"}};

    //     auto agg = std::make_unique<GlobalAggOperator>(std::move(scan), std::move(specs));

    //     auto start = std::chrono::steady_clock::now();
    //     Batch* output_batch = agg->next();
    //     auto end = std::chrono::steady_clock::now();

    //     for (size_t i = 0; i < output_batch->column_count_; ++i) {
    //         std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
    //     }

    //     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "\nTime: " << ms << " ms" << std::endl;
    // }

    // // ==============
    // // Query 6
    // // ==============
    // {
    //     std::cout << "\nQuery 6" << std::endl;

    //     auto scan =
    //         std::make_unique<ScanOperator>(output_file,
    //         std::vector<std::string>{"SearchPhrase"});

    //     std::vector<AggSpec> specs{{AggType::StrCountDistinct, "SearchPhrase"}};

    //     auto agg = std::make_unique<GlobalAggOperator>(std::move(scan), std::move(specs));

    //     auto start = std::chrono::steady_clock::now();
    //     Batch* output_batch = agg->next();
    //     auto end = std::chrono::steady_clock::now();

    //     for (size_t i = 0; i < output_batch->column_count_; ++i) {
    //         std::cout << output_batch->columns_[i].get_value<int64_t>(0) << ' ';
    //     }

    //     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "\nTime: " << ms << " ms" << std::endl;
    // }

    // // ==============
    // // Query 7
    // // ==============
    // {
    //     std::cout << "\nQuery 7" << std::endl;

    //     auto scan =
    //         std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"EventDate"});

    //     std::vector<AggSpec> specs{{AggType::Min, "EventDate"}, {AggType::Max, "EventDate"}};

    //     auto agg = std::make_unique<GlobalAggOperator>(std::move(scan), std::move(specs));

    //     auto start = std::chrono::steady_clock::now();
    //     Batch* output_batch = agg->next();
    //     auto end = std::chrono::steady_clock::now();

    //     for (size_t i = 0; i < output_batch->column_count_; ++i) {
    //         std::cout << CsvWriter::convert_to_date(
    //                          static_cast<int32_t>(output_batch->columns_[i].get_value<int64_t>(0)))
    //                   << ' ';
    //     }

    //     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "\nTime: " << ms << " ms" << std::endl;
    // }

    return 0;
}

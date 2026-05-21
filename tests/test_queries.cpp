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

using columnar::AggSpec;
using columnar::AggType;
using columnar::Batch;
using columnar::ColumnarWriter;
using columnar::ColumnRef;
using columnar::ConversionBatch;
using columnar::CsvReader;
using columnar::CsvWriter;
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
    // Query 1:
    // SELECT COUNT(*) FROM hits;
    // ==============
    {
        std::cout << "Query 1" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{}),
            make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 2:
    // SELECT COUNT(*) FROM hits WHERE AdvEngineID <> 0;
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

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 3:
    // SELECT SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth) FROM hits;
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

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 4:
    // SELECT AVG(UserID) FROM hits;
    // ==============
    {
        std::cout << "\nQuery 4" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"UserID"}),
            make_aggs(AggSpec(AggType::Avg, "UserID", std::make_unique<ColumnRef>("UserID"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 5:
    // SELECT COUNT(DISTINCT UserID) FROM hits;
    // ==============
    {
        std::cout << "\nQuery 5" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"UserID"}),
            make_aggs(
                AggSpec(AggType::CountDistinct, "UserID", std::make_unique<ColumnRef>("UserID"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 6:
    // SELECT COUNT(DISTINCT SearchPhrase) FROM hits;
    // ==============
    {
        std::cout << "\nQuery 6" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"SearchPhrase"}),
            make_aggs(AggSpec(AggType::CountDistinct, "SearchPhrase",
                              std::make_unique<ColumnRef>("SearchPhrase"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 7:
    // SELECT MIN(EventDate), MAX(EventDate) FROM hits;
    // ==============
    {
        std::cout << "\nQuery 7" << std::endl;

        auto plan = std::make_unique<GlobalAggOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"EventDate"}),
            make_aggs(
                AggSpec(AggType::Min, "EventDate", std::make_unique<ColumnRef>("EventDate")),
                AggSpec(AggType::Max, "EventDate", std::make_unique<ColumnRef>("EventDate"))));

        auto start = std::chrono::steady_clock::now();
        Batch* output_batch = plan->next();
        auto end = std::chrono::steady_clock::now();

        CsvWriter::write_batch(*output_batch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    return 0;
}

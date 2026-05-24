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
using columnar::Compare;
using columnar::ConversionBatch;
using columnar::CsvReader;
using columnar::CsvWriter;
using columnar::FilterOperator;
using columnar::GlobalAggOperator;
using columnar::GroupByOperator;
using columnar::IValueExpression;
using columnar::LimitOperator;
using columnar::Literal;
using columnar::OrderByOperator;
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

template <typename... Ts>
std::vector<std::unique_ptr<IValueExpression>> make_groups(Ts&&... ts) {
    std::vector<std::unique_ptr<IValueExpression>> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

template <typename... Ts>
std::vector<OrderByOperator::OrderSpec> order_specs(Ts&&... ts) {
    std::vector<OrderByOperator::OrderSpec> v;
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
                std::make_unique<Compare>(std::make_unique<ColumnRef>("AdvEngineID"),
                                          std::make_unique<Literal>(0), Compare::Cmp::NE)),
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

    // ==============
    // Query 8:
    // SELECT AdvEngineID, COUNT(*) FROM hits WHERE AdvEngineID <> 0 GROUP BY AdvEngineID ORDER BY
    // COUNT(*) DESC;
    // ==============
    {
        std::cout << "\nQuery 8" << std::endl;

        auto plan = std::make_unique<OrderByOperator>(
            std::make_unique<GroupByOperator>(
                std::make_unique<FilterOperator>(
                    std::make_unique<ScanOperator>(output_file,
                                                   std::vector<std::string>{"AdvEngineID"}),
                    std::make_unique<Compare>(std::make_unique<ColumnRef>("AdvEngineID"),
                                              std::make_unique<Literal>(0), Compare::Cmp::NE)),
                make_groups(std::make_unique<ColumnRef>("AdvEngineID")),
                make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*")))),
            order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("*"),
                                                   OrderByOperator::OrderDirection::Desc}));

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 9:
    // SELECT RegionID, COUNT(DISTINCT UserID) AS u FROM hits GROUP BY RegionID ORDER BY u DESC
    // LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 9" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<ScanOperator>(output_file,
                                                   std::vector<std::string>{"RegionID", "UserID"}),
                    make_groups(std::make_unique<ColumnRef>("RegionID")),
                    make_aggs(AggSpec(AggType::CountDistinct, "UserID",
                                      std::make_unique<ColumnRef>("UserID"), "u"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("u"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 10:
    // SELECT RegionID, SUM(AdvEngineID), COUNT(*) AS c, AVG(ResolutionWidth), COUNT(DISTINCT
    // UserID) FROM hits GROUP BY RegionID ORDER BY c DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 10" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<ScanOperator>(
                        output_file, std::vector<std::string>{"RegionID", "AdvEngineID",
                                                              "ResolutionWidth", "UserID"}),
                    make_groups(std::make_unique<ColumnRef>("RegionID")),
                    make_aggs(AggSpec(AggType::Sum, "AdvEngineID",
                                      std::make_unique<ColumnRef>("AdvEngineID")),
                              AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"),
                              AggSpec(AggType::Avg, "ResolutionWidth",
                                      std::make_unique<ColumnRef>("ResolutionWidth")),
                              AggSpec(AggType::CountDistinct, "UserID",
                                      std::make_unique<ColumnRef>("UserID")))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 11:
    // SELECT MobilePhoneModel, COUNT(DISTINCT UserID) AS u FROM hits WHERE MobilePhoneModel <> ''
    // GROUP BY MobilePhoneModel ORDER BY u DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 11" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<FilterOperator>(
                        std::make_unique<ScanOperator>(
                            output_file, std::vector<std::string>{"MobilePhoneModel", "UserID"}),
                        std::make_unique<Compare>(std::make_unique<ColumnRef>("MobilePhoneModel"),
                                                  std::make_unique<Literal>(""),
                                                  columnar::Compare::Cmp::NE)),
                    make_groups(std::make_unique<ColumnRef>("MobilePhoneModel")),
                    make_aggs(AggSpec(AggType::CountDistinct, "UserID",
                                      std::make_unique<ColumnRef>("UserID"), "u"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("u"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 12:
    // SELECT MobilePhone, MobilePhoneModel, COUNT(DISTINCT UserID) AS u FROM hits WHERE
    // MobilePhoneModel <> '' GROUP BY MobilePhone, MobilePhoneModel ORDER BY u DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 12" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<FilterOperator>(
                        std::make_unique<ScanOperator>(
                            output_file,
                            std::vector<std::string>{"MobilePhone", "MobilePhoneModel", "UserID"}),
                        std::make_unique<Compare>(std::make_unique<ColumnRef>("MobilePhoneModel"),
                                                  std::make_unique<Literal>(""), Compare::Cmp::NE)),
                    make_groups(std::make_unique<ColumnRef>("MobilePhone"),
                                std::make_unique<ColumnRef>("MobilePhoneModel")),
                    make_aggs(AggSpec(AggType::CountDistinct, "UserID",
                                      std::make_unique<ColumnRef>("UserID"), "u"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("u"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 13:
    // SELECT SearchPhrase, COUNT(*) AS c FROM hits WHERE SearchPhrase <> '' GROUP BY SearchPhrase
    // ORDER BY c DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 13" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<FilterOperator>(
                        std::make_unique<ScanOperator>(output_file,
                                                       std::vector<std::string>{"SearchPhrase"}),
                        std::make_unique<Compare>(std::make_unique<ColumnRef>("SearchPhrase"),
                                                  std::make_unique<Literal>(""), Compare::Cmp::NE)),
                    make_groups(std::make_unique<ColumnRef>("SearchPhrase")),
                    make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 14:
    // SELECT SearchPhrase, COUNT(DISTINCT UserID) AS u
    // FROM hits WHERE SearchPhrase <> ''
    // GROUP BY SearchPhrase ORDER BY u DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 14" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<FilterOperator>(
                        std::make_unique<ScanOperator>(
                            output_file, std::vector<std::string>{"SearchPhrase", "UserID"}),
                        std::make_unique<Compare>(std::make_unique<ColumnRef>("SearchPhrase"),
                                                  std::make_unique<Literal>(""), Compare::Cmp::NE)),
                    make_groups(std::make_unique<ColumnRef>("SearchPhrase")),
                    make_aggs(AggSpec(AggType::CountDistinct, "UserID",
                                      std::make_unique<ColumnRef>("UserID"), "u"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("u"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 15:
    // SELECT SearchEngineID, SearchPhrase, COUNT(*) AS c
    // FROM hits WHERE SearchPhrase <> ''
    // GROUP BY SearchEngineID, SearchPhrase
    // ORDER BY c DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 15" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<FilterOperator>(
                        std::make_unique<ScanOperator>(
                            output_file,
                            std::vector<std::string>{"SearchEngineID", "SearchPhrase"}),
                        std::make_unique<Compare>(std::make_unique<ColumnRef>("SearchPhrase"),
                                                  std::make_unique<Literal>(""), Compare::Cmp::NE)),
                    make_groups(std::make_unique<ColumnRef>("SearchEngineID"),
                                std::make_unique<ColumnRef>("SearchPhrase")),
                    make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 16:
    // SELECT UserID, COUNT(*) FROM hits
    // GROUP BY UserID ORDER BY COUNT(*) DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 16" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"UserID"}),
                    make_groups(std::make_unique<ColumnRef>("UserID")),
                    make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 17:
    // SELECT UserID, SearchPhrase, COUNT(*)
    // FROM hits
    // GROUP BY UserID, SearchPhrase
    // ORDER BY COUNT(*) DESC LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 17" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<OrderByOperator>(
                std::make_unique<GroupByOperator>(
                    std::make_unique<ScanOperator>(
                        output_file, std::vector<std::string>{"UserID", "SearchPhrase"}),
                    make_groups(std::make_unique<ColumnRef>("UserID"),
                                std::make_unique<ColumnRef>("SearchPhrase")),
                    make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
                order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
                                                       OrderByOperator::OrderDirection::Desc})),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 18:
    // SELECT UserID, SearchPhrase, COUNT(*)
    // FROM hits
    // GROUP BY UserID, SearchPhrase
    // LIMIT 10;
    // ==============
    {
        std::cout << "\nQuery 18" << std::endl;

        auto plan = std::make_unique<LimitOperator>(
            std::make_unique<GroupByOperator>(
                std::make_unique<ScanOperator>(output_file,
                                               std::vector<std::string>{"UserID", "SearchPhrase"}),
                make_groups(std::make_unique<ColumnRef>("UserID"),
                            std::make_unique<ColumnRef>("SearchPhrase")),
                make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
            10);

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    // ==============
    // Query 19:
    // SELECT UserID,
    //        extract(minute FROM EventTime) AS m,
    //        SearchPhrase,
    //        COUNT(*)
    // FROM hits
    // GROUP BY UserID, m, SearchPhrase
    // ORDER BY COUNT(*) DESC LIMIT 10;
    // ==============
    // {
    //     std::cout << "\nQuery 19" << std::endl;

    //     auto plan = std::make_unique<LimitOperator>(
    //         std::make_unique<OrderByOperator>(
    //             std::make_unique<GroupByOperator>(
    //                 std::make_unique<ScanOperator>(
    //                     output_file,
    //                     std::vector<std::string>{"UserID", "EventTime", "SearchPhrase"}),
    //                 make_groups(
    //                     std::make_unique<ColumnRef>("UserID"),
    //                     std::make_unique<ExtractMinute>(std::make_unique<ColumnRef>("EventTime")),
    //                     std::make_unique<ColumnRef>("SearchPhrase")),
    //                 make_aggs(AggSpec(AggType::Count, "*", std::make_unique<ColumnRef>("*"), "c"))),
    //             order_specs(OrderByOperator::OrderSpec{std::make_unique<ColumnRef>("c"),
    //                                                    OrderByOperator::OrderDirection::Desc})),
    //         10);

    //     auto start = std::chrono::steady_clock::now();
    //     while (Batch* output_batch = plan->next()) {
    //         CsvWriter::write_batch(*output_batch);
    //     }
    //     auto end = std::chrono::steady_clock::now();

    //     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "\nTime: " << ms << " ms" << std::endl;
    // }

    // ==============
    // Query 20:
    // SELECT UserID FROM hits WHERE UserID = 435090932899640449;
    // ==============
    {
        std::cout << "\nQuery 20" << std::endl;

        auto plan = std::make_unique<FilterOperator>(
            std::make_unique<ScanOperator>(output_file, std::vector<std::string>{"UserID"}),
            std::make_unique<Compare>(std::make_unique<ColumnRef>("UserID"),
                                      std::make_unique<Literal>(435090932899640449LL),
                                      Compare::Cmp::E));

        auto start = std::chrono::steady_clock::now();
        while (Batch* output_batch = plan->next()) {
            CsvWriter::write_batch(*output_batch);
        }
        auto end = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nTime: " << ms << " ms" << std::endl;
    }

    return 0;
}

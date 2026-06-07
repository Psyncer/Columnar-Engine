#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "src/storage/batch.hpp"
#include "src/io/columnar_writer.hpp"
#include "src/storage/conversion_batch.hpp"
#include "src/io/csv_reader.hpp"
#include "src/io/csv_writer.hpp"
#include "src/execution/expression.hpp"
#include "src/execution/operator.hpp"
#include "src/io/parsing.hpp"
#include "src/runner/query_builder.hpp"

using namespace columnar;

void query00(const std::string& columnar_path) {
    // ==============
    // SELECT COUNT(*)
    // FROM hits;
    // ==============

    std::cerr << "Query 0\n";

    QueryBuilder q(columnar_path);

    auto plan = q.global_agg(q.scan({""}), make_aggs(count("*")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query01(const std::string& columnar_path) {
    // ==============
    // SELECT COUNT(*)
    // FROM hits
    // WHERE AdvEngineID <> 0;
    // ==============

    std::cerr << "Query 1\n";

    QueryBuilder q(columnar_path);

    auto plan =
        q.global_agg(q.filter(q.scan({"AdvEngineID"}), not_equal(col("AdvEngineID"), lit(0))),
                     make_aggs(count("*")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query02(const std::string& columnar_path) {
    // ==============
    // SELECT SUM(AdvEngineID)
    // SELECT COUNT(*)
    // SELECT AVG(ResolutionWidth)
    // FROM hits;
    // ==============

    std::cerr << "Query 2\n";

    QueryBuilder q(columnar_path);

    auto plan = q.global_agg(q.scan({"AdvEngineID", "ResolutionWidth"}),
                             make_aggs(sum("AdvEngineID"), count("*"), avg("ResolutionWidth")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query03(const std::string& columnar_path) {
    // ==============
    // SELECT AVG(UserID)
    // FROM hits;
    // ==============

    std::cerr << "Query 3\n";

    QueryBuilder q(columnar_path);

    auto plan = q.global_agg(q.scan({"UserID"}), make_aggs(avg("UserID")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query04(const std::string& columnar_path) {
    // ==============
    // SELECT COUNT(DISTINCT UserID)
    // FROM hits;
    // ==============

    std::cerr << "Query 4\n";

    QueryBuilder q(columnar_path);

    auto plan = q.global_agg(q.scan({"UserID"}), make_aggs(count_distinct("UserID")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query05(const std::string& columnar_path) {
    // ==============
    // SELECT COUNT(DISTINCT SearchPhrase)
    // FROM hits;
    // ==============

    std::cerr << "Query 5\n";

    QueryBuilder q(columnar_path);

    auto plan =
        q.global_agg(q.scan({"SearchPhrase"}), make_aggs(str_count_distinct("SearchPhrase")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query06(const std::string& columnar_path) {
    // ==============
    // SELECT MIN(EventDate)
    // SELECT MAX(EventDate)
    // FROM hits;
    // ==============

    std::cerr << "Query 6\n";

    QueryBuilder q(columnar_path);

    auto plan = q.global_agg(q.scan({"EventDate"}), make_aggs(min("EventDate"), max("EventDate")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query07(const std::string& columnar_path) {
    // ==============
    // SELECT AdvEngineID
    // SELECT COUNT(*)
    // FROM hits
    // WHERE AdvEngineID <> 0
    // GROUP BY AdvEngineID
    // ORDER BY COUNT(*) DESC;
    // ==============

    std::cerr << "Query 7\n";

    QueryBuilder q(columnar_path);

    auto plan = q.order_by(
        q.group_by(q.filter(q.scan({"AdvEngineID"}), not_equal(col("AdvEngineID"), lit(0))),
                   make_groups(col("AdvEngineID")), make_aggs(count("*"))),
        order_specs(OrderByOperator::OrderSpec{col("*"), OrderByOperator::OrderDirection::Desc}));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query08(const std::string& columnar_path) {
    // ==============
    // SELECT RegionID
    // SELECT COUNT(DISTINCT UserID)
    // FROM hits
    // GROUP BY RegionID
    // ORDER BY u DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 8\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"RegionID", "UserID"}), make_groups(col("RegionID")),
                   make_aggs(count_distinct("UserID", "u"))),
        top_k_specs(TopKOperator::OrderSpec{col("u"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query09(const std::string& columnar_path) {
    // ==============
    // SELECT RegionID
    // SELECT SUM(AdvEngineID)
    // SELECT COUNT(*) AS c
    // SELECT AVG(ResolutionWidth)
    // SELECT COUNT(DISTINCT UserID)
    // FROM hits
    // GROUP BY RegionID
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 9\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"RegionID", "AdvEngineID", "ResolutionWidth", "UserID"}),
                   make_groups(col("RegionID")),
                   make_aggs(sum("AdvEngineID"), count("*", "c"), avg("ResolutionWidth"),
                             count_distinct("UserID"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query10(const std::string& columnar_path) {
    // ==============
    // SELECT MobilePhoneModel, COUNT(DISTINCT UserID) AS u
    // FROM hits
    // WHERE MobilePhoneModel <> ''
    // GROUP BY MobilePhoneModel
    // ORDER BY u DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 10\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"MobilePhoneModel", "UserID"}),
                            not_equal(col("MobilePhoneModel"), lit(""))),
                   make_groups(col("MobilePhoneModel")), make_aggs(count_distinct("UserID", "u"))),
        top_k_specs(TopKOperator::OrderSpec{col("u"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query11(const std::string& columnar_path) {
    // ==============
    // SELECT MobilePhone, MobilePhoneModel, COUNT(DISTINCT UserID) AS u
    // FROM hits
    // WHERE MobilePhoneModel <> ''
    // GROUP BY MobilePhone, MobilePhoneModel
    // ORDER BY u DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 11\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"MobilePhone", "MobilePhoneModel", "UserID"}),
                            not_equal(col("MobilePhoneModel"), lit(""))),
                   make_groups(col("MobilePhone"), col("MobilePhoneModel")),
                   make_aggs(count_distinct("UserID", "u"))),
        top_k_specs(TopKOperator::OrderSpec{col("u"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query12(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase, COUNT(*) AS c
    // FROM hits
    // WHERE SearchPhrase <> ''
    // GROUP BY SearchPhrase
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 12\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"SearchPhrase"}), not_equal(col("SearchPhrase"), lit(""))),
                   make_groups(col("SearchPhrase")), make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query13(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase, COUNT(DISTINCT UserID) AS u
    // FROM hits
    // WHERE SearchPhrase <> ''
    // GROUP BY SearchPhrase
    // ORDER BY u DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 13\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(
            q.filter(q.scan({"SearchPhrase", "UserID"}), not_equal(col("SearchPhrase"), lit(""))),
            make_groups(col("SearchPhrase")), make_aggs(count_distinct("UserID", "u"))),
        top_k_specs(TopKOperator::OrderSpec{col("u"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query14(const std::string& columnar_path) {
    // ==============
    // SELECT SearchEngineID, SearchPhrase, COUNT(*) AS c
    // FROM hits
    // WHERE SearchPhrase <> ''
    // GROUP BY SearchEngineID, SearchPhrase
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 14\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"SearchEngineID", "SearchPhrase"}),
                            not_equal(col("SearchPhrase"), lit(""))),
                   make_groups(col("SearchEngineID"), col("SearchPhrase")),
                   make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query15(const std::string& columnar_path) {
    // ==============
    // SELECT UserID, COUNT(*)
    // FROM hits
    // GROUP BY UserID
    // ORDER BY COUNT(*) DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 15\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"UserID"}), make_groups(col("UserID")), make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query16(const std::string& columnar_path) {
    // ==============
    // SELECT UserID, SearchPhrase, COUNT(*)
    // FROM hits
    // GROUP BY UserID, SearchPhrase
    // ORDER BY COUNT(*) DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 16\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"UserID", "SearchPhrase"}),
                   make_groups(col("UserID"), col("SearchPhrase")), make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query17(const std::string& columnar_path) {
    // ==============
    // SELECT UserID, SearchPhrase, COUNT(*)
    // FROM hits
    // GROUP BY UserID, SearchPhrase
    // LIMIT 10;
    // ==============

    std::cerr << "Query 17\n";

    QueryBuilder q(columnar_path);

    auto plan = q.limit(q.group_by(q.scan({"UserID", "SearchPhrase"}),
                                   make_groups(col("UserID"), col("SearchPhrase")),
                                   make_aggs(count("*", "c"))),
                        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query18(const std::string& columnar_path) {
    // ==============
    // SELECT UserID, extract(minute FROM EventTime) AS m, SearchPhrase, COUNT(*)
    // FROM hits
    // GROUP BY UserID, m, SearchPhrase
    // ORDER BY COUNT(*) DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 18\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"UserID", "EventTime", "SearchPhrase"}),
                   make_groups(col("UserID"),
                               col("m", extract(col("EventTime"), Extract::ExtractSpec::minute)),
                               col("SearchPhrase")),
                   make_aggs(count("*"))),
        top_k_specs(TopKOperator::OrderSpec{col("*"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query19(const std::string& columnar_path) {
    // ==============
    // SELECT UserID
    // FROM hits
    // WHERE UserID = 435090932899640449;
    // ==============

    std::cerr << "Query 19\n";

    QueryBuilder q(columnar_path);

    auto plan = q.filter(q.scan({"UserID"}), equal(col("UserID"), lit(435090932899640449LL)));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query20(const std::string& columnar_path) {
    // ==============
    // SELECT COUNT(*)
    // FROM hits
    // WHERE URL LIKE '%google%';
    // ==============

    std::cerr << "Query 20\n";

    QueryBuilder q(columnar_path);

    auto plan =
        q.global_agg(q.filter(q.scan({"URL"}), std::make_unique<Like>(col("URL"), "%google%")),
                     make_aggs(count("*")));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query21(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase, MIN(URL), COUNT(*) AS c
    // FROM hits
    // WHERE URL LIKE '%google%' AND SearchPhrase <> ''
    // GROUP BY SearchPhrase
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 21\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"SearchPhrase", "URL"}),
                            and_expr(like(col("URL"), "%google%"),
                                     not_equal(col("SearchPhrase"), lit("")))),
                   make_groups(col("SearchPhrase")), make_aggs(str_min("URL"), count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query22(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase, MIN(URL), MIN(Title), COUNT(*) AS c, COUNT(DISTINCT UserID)
    // FROM hits
    // WHERE Title LIKE '%Google%' AND URL NOT LIKE '%.google.%' AND SearchPhrase <> ''
    // GROUP BY SearchPhrase
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 22\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(
            q.filter(q.scan({"SearchPhrase", "URL", "Title", "UserID"}),
                     and_expr(and_expr(like(col("Title"), "%Google%"),
                                       not_like(col("URL"), "%.google.%")),
                              not_equal(col("SearchPhrase"), lit("")))),
            make_groups(col("SearchPhrase")),
            make_aggs(str_min("URL"), str_min("Title"), count("*", "c"), count_distinct("UserID"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query23(const std::string& columnar_path) {
    // ==============
    // SELECT *
    // FROM hits
    // WHERE URL LIKE '%google%'
    // ORDER BY EventTime
    // LIMIT 10;
    // ==============

    std::cerr << "Query 23\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.filter(q.scan({"*"}), like(col("URL"), "%google%")),
        top_k_specs(TopKOperator::OrderSpec{col("EventTime"), TopKOperator::OrderDirection::Asc}),
        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query24(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase
    // FROM hits
    // WHERE SearchPhrase <> ''
    // ORDER BY EventTime
    // LIMIT 10;
    // ==============

    std::cerr << "Query 24\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.filter(q.scan({"SearchPhrase", "EventTime"}), not_equal(col("SearchPhrase"), lit(""))),
        top_k_specs(TopKOperator::OrderSpec{col("EventTime"), TopKOperator::OrderDirection::Asc}),
        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query25(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase
    // FROM hits
    // WHERE SearchPhrase <> ''
    // ORDER BY SearchPhrase
    // LIMIT 10;
    // ==============

    std::cerr << "Query 25\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(q.filter(q.scan({"SearchPhrase"}), not_equal(col("SearchPhrase"), lit(""))),
                        top_k_specs(TopKOperator::OrderSpec{col("SearchPhrase"),
                                                            TopKOperator::OrderDirection::Asc}),
                        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query26(const std::string& columnar_path) {
    // ==============
    // SELECT SearchPhrase
    // FROM hits
    // WHERE SearchPhrase <> ''
    // ORDER BY EventTime, SearchPhrase
    // LIMIT 10;
    // ==============

    std::cerr << "Query 26\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.filter(q.scan({"SearchPhrase", "EventTime"}), not_equal(col("SearchPhrase"), lit(""))),
        top_k_specs(
            TopKOperator::OrderSpec{col("EventTime"), TopKOperator::OrderDirection::Asc},
            TopKOperator::OrderSpec{col("SearchPhrase"), TopKOperator::OrderDirection::Asc}),
        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query27(const std::string& columnar_path) {
    // ==============
    // SELECT CounterID, AVG(STRLEN(URL)) AS l, COUNT(*) AS c
    // FROM hits
    // WHERE URL <> ''
    // GROUP BY CounterID
    // HAVING COUNT(*) > 100000
    // ORDER BY l DESC
    // LIMIT 25;
    // ==============

    std::cerr << "Query 27\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.having(q.group_by(q.filter(q.scan({"CounterID", "URL"}), not_equal(col("URL"), lit(""))),
                            make_groups(col("CounterID")),
                            make_aggs(avg("URL", "l", str_len(col("URL"))), count("*", "c"))),
                 greater(col("c"), lit(100000))),
        top_k_specs(TopKOperator::OrderSpec{col("l"), TopKOperator::OrderDirection::Desc}), 25);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query28(const std::string& columnar_path) {
    // ==============
    // SELECT REGEXP_REPLACE(Referer, '^https?://(?:www\\.)?([^/]+)/.*$', '\\1') AS k,
    //      AVG(STRLEN(Referer)) AS l,
    //      COUNT(*) AS c,
    //      MIN(Referer)
    // FROM hits
    // WHERE Referer <> ''
    // GROUP BY k
    // HAVING COUNT(*) > 100000
    // ORDER BY l DESC
    // LIMIT 25;
    // ==============

    std::cerr << "Query 28\n";

    QueryBuilder q(columnar_path);

    auto k_expr = regexp_replace(col("Referer"), "^https?://(?:www\\.)?([^/]+)/.*$", "\\1");

    auto plan = q.top_k(
        q.having(q.group_by(q.filter(q.scan({"Referer"}), not_equal(col("Referer"), lit(""))),
                            make_groups(col("k", std::move(k_expr))),
                            make_aggs(avg("Referer", "l", str_len(col("Referer"))), count("*", "c"),
                                      str_min("Referer"))),
                 greater(col("c"), lit(100000))),
        top_k_specs(TopKOperator::OrderSpec{col("l"), TopKOperator::OrderDirection::Desc}), 25);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query29(const std::string& columnar_path) {
    // ==============
    // SELECT SUM(ResolutionWidth), SUM(ResolutionWidth + 1), ..., SUM(ResolutionWidth + 89)
    // FROM hits;
    // ==============

    std::cerr << "Query 29\n";

    QueryBuilder q(columnar_path);

    auto scan = q.scan({"ResolutionWidth"});

    auto plan =
        q.global_agg(std::move(scan),
                     make_aggs(sum("ResolutionWidth"),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(1))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(2))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(3))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(4))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(5))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(6))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(7))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(8))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(9))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(10))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(11))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(12))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(13))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(14))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(15))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(16))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(17))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(18))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(19))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(20))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(21))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(22))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(23))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(24))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(25))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(26))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(27))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(28))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(29))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(30))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(31))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(32))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(33))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(34))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(35))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(36))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(37))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(38))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(39))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(40))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(41))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(42))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(43))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(44))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(45))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(46))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(47))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(48))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(49))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(50))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(51))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(52))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(53))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(54))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(55))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(56))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(57))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(58))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(59))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(60))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(61))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(62))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(63))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(64))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(65))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(66))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(67))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(68))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(69))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(70))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(71))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(72))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(73))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(74))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(75))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(76))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(77))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(78))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(79))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(80))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(81))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(82))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(83))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(84))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(85))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(86))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(87))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(88))),
                               sum("ResolutionWidth", "", add(col("ResolutionWidth"), lit(89)))));

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query30(const std::string& columnar_path) {
    // ==============
    // SELECT SearchEngineID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth)
    // FROM hits
    // WHERE SearchPhrase <> ''
    // GROUP BY SearchEngineID, ClientIP
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 30\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"SearchEngineID", "ClientIP", "IsRefresh", "ResolutionWidth",
                                    "SearchPhrase"}),
                            not_equal(col("SearchPhrase"), lit(""))),
                   make_groups(col("SearchEngineID"), col("ClientIP")),
                   make_aggs(count("*", "c"), sum("IsRefresh"), avg("ResolutionWidth"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query31(const std::string& columnar_path) {
    // ==============
    // SELECT WatchID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth)
    // FROM hits
    // WHERE SearchPhrase <> ''
    // GROUP BY WatchID, ClientIP
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 31\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"WatchID", "ClientIP", "IsRefresh", "ResolutionWidth",
                                    "SearchPhrase"}),
                            not_equal(col("SearchPhrase"), lit(""))),
                   make_groups(col("WatchID"), col("ClientIP")),
                   make_aggs(count("*", "c"), sum("IsRefresh"), avg("ResolutionWidth"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query32(const std::string& columnar_path) {
    // ==============
    // SELECT WatchID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth)
    // FROM hits
    // GROUP BY WatchID, ClientIP
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 32\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"WatchID", "ClientIP", "IsRefresh", "ResolutionWidth"}),
                   make_groups(col("WatchID"), col("ClientIP")),
                   make_aggs(count("*", "c"), sum("IsRefresh"), avg("ResolutionWidth"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query33(const std::string& columnar_path) {
    // ==============
    // SELECT URL, COUNT(*) AS c
    // FROM hits
    // GROUP BY URL
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 33\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"URL"}), make_groups(col("URL")), make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query34(const std::string& columnar_path) {
    // ==============
    // SELECT 1, URL, COUNT(*) AS c
    // FROM hits
    // GROUP BY 1, URL
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 34\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"URL"}), make_groups(col("one", lit(1)), col("URL")),
                   make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query35(const std::string& columnar_path) {
    // ==============
    // SELECT ClientIP, ClientIP - 1, ClientIP - 2, ClientIP - 3, COUNT(*) AS c
    // FROM hits
    // GROUP BY ClientIP, ClientIP - 1, ClientIP - 2, ClientIP - 3
    // ORDER BY c DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 35\n";

    QueryBuilder q(columnar_path);

    auto plan = q.top_k(
        q.group_by(q.scan({"ClientIP"}),
                   make_groups(col("ClientIP"), col("ClientIP_m1", sub(col("ClientIP"), lit(1))),
                               col("ClientIP_m2", sub(col("ClientIP"), lit(2))),
                               col("ClientIP_m3", sub(col("ClientIP"), lit(3)))),
                   make_aggs(count("*", "c"))),
        top_k_specs(TopKOperator::OrderSpec{col("c"), TopKOperator::OrderDirection::Desc}), 10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query36(const std::string& columnar_path) {
    // ==============
    // SELECT URL, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND DontCountHits = 0
    //   AND IsRefresh = 0
    //   AND URL <> ''
    // GROUP BY URL
    // ORDER BY PageViews DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 36\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                                   equal(col("DontCountHits"), lit(0)))),
                 and_expr(equal(col("IsRefresh"), lit(0)), not_equal(col("URL"), lit(""))));

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"URL", "CounterID", "EventDate", "DontCountHits", "IsRefresh"}),
                            std::move(predicate)),
                   make_groups(col("URL")), make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query37(const std::string& columnar_path) {
    // ==============
    // SELECT Title, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND DontCountHits = 0
    //   AND IsRefresh = 0
    //   AND Title <> ''
    // GROUP BY Title
    // ORDER BY PageViews DESC
    // LIMIT 10;
    // ==============

    std::cerr << "Query 37\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                                   equal(col("DontCountHits"), lit(0)))),
                 and_expr(equal(col("IsRefresh"), lit(0)), not_equal(col("Title"), lit(""))));

    auto plan = q.top_k(
        q.group_by(
            q.filter(q.scan({"Title", "CounterID", "EventDate", "DontCountHits", "IsRefresh"}),
                     std::move(predicate)),
            make_groups(col("Title")), make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query38(const std::string& columnar_path) {
    // ==============
    // SELECT URL, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND IsRefresh = 0
    //   AND IsLink <> 0
    //   AND IsDownload = 0
    // GROUP BY URL
    // ORDER BY PageViews DESC
    // LIMIT 10 OFFSET 1000;
    // ==============

    std::cerr << "Query 38\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                                   equal(col("IsRefresh"), lit(0)))),
                 and_expr(not_equal(col("IsLink"), lit(0)), equal(col("IsDownload"), lit(0))));

    auto plan = q.top_k(
        q.group_by(
            q.filter(q.scan({"URL", "CounterID", "EventDate", "IsRefresh", "IsLink", "IsDownload"}),
                     std::move(predicate)),
            make_groups(col("URL")), make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10, 1000);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query39(const std::string& columnar_path) {
    // ==============
    // SELECT TraficSourceID, SearchEngineID, AdvEngineID,
    //        CASE WHEN (SearchEngineID = 0 AND AdvEngineID = 0)
    //             THEN Referer ELSE '' END AS Src,
    //        URL AS Dst,
    //        COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND IsRefresh = 0
    // GROUP BY TraficSourceID, SearchEngineID, AdvEngineID, Src, Dst
    // ORDER BY PageViews DESC
    // LIMIT 10 OFFSET 1000;
    // ==============

    std::cerr << "Query 39\n";

    QueryBuilder q(columnar_path);

    auto src_expr =
        case_when(and_expr(equal(col("SearchEngineID"), lit(0)), equal(col("AdvEngineID"), lit(0))),
                  col("Referer"), lit(""));

    auto predicate =
        and_expr(and_expr(equal(col("CounterID"), lit(62)),
                          greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                 and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                          equal(col("IsRefresh"), lit(0))));

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"TraficSourceID", "SearchEngineID", "AdvEngineID", "Referer",
                                    "URL", "CounterID", "EventDate", "IsRefresh"}),
                            std::move(predicate)),
                   make_groups(col("TraficSourceID"), col("SearchEngineID"), col("AdvEngineID"),
                               col("Src", std::move(src_expr)), col("Dst", col("URL"))),
                   make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10, 1000);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query40(const std::string& columnar_path) {
    // ==============
    // SELECT URLHash, EventDate, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND IsRefresh = 0
    //   AND TraficSourceID IN (-1, 6)
    //   AND RefererHash = 3594120000172545465
    // GROUP BY URLHash, EventDate
    // ORDER BY PageViews DESC
    // LIMIT 10 OFFSET 100;
    // ==============

    std::cerr << "Query 40\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                                   equal(col("IsRefresh"), lit(0)))),
                 and_expr(in(col("TraficSourceID"), {-1, 6}),
                          equal(col("RefererHash"), lit(3594120000172545465LL))));

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"URLHash", "EventDate", "CounterID", "IsRefresh",
                                    "TraficSourceID", "RefererHash"}),
                            std::move(predicate)),
                   make_groups(col("URLHash"), col("EventDate")),
                   make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10, 100);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query41(const std::string& columnar_path) {
    // ==============
    // SELECT WindowClientWidth, WindowClientHeight, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-01'
    //   AND EventDate <= '2013-07-31'
    //   AND IsRefresh = 0
    //   AND DontCountHits = 0
    //   AND URLHash = 2868770270353813622
    // GROUP BY WindowClientWidth, WindowClientHeight
    // ORDER BY PageViews DESC
    // LIMIT 10 OFFSET 10000;
    // ==============

    std::cerr << "Query 41\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-01")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-31"))),
                                   equal(col("IsRefresh"), lit(0)))),
                 and_expr(equal(col("DontCountHits"), lit(0)),
                          equal(col("URLHash"), lit(2868770270353813622LL))));

    auto plan = q.top_k(
        q.group_by(q.filter(q.scan({"WindowClientWidth", "WindowClientHeight", "CounterID",
                                    "EventDate", "IsRefresh", "DontCountHits", "URLHash"}),
                            std::move(predicate)),
                   make_groups(col("WindowClientWidth"), col("WindowClientHeight")),
                   make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("PageViews"), TopKOperator::OrderDirection::Desc}),
        10, 10000);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void query42(const std::string& columnar_path) {
    // ==============
    // SELECT DATE_TRUNC('minute', EventTime) AS M, COUNT(*) AS PageViews
    // FROM hits
    // WHERE CounterID = 62
    //   AND EventDate >= '2013-07-14'
    //   AND EventDate <= '2013-07-15'
    //   AND IsRefresh = 0
    //   AND DontCountHits = 0
    // GROUP BY DATE_TRUNC('minute', EventTime)
    // ORDER BY DATE_TRUNC('minute', EventTime)
    // LIMIT 10 OFFSET 1000;
    // ==============

    std::cerr << "Query 42\n";

    QueryBuilder q(columnar_path);

    auto predicate =
        and_expr(and_expr(and_expr(equal(col("CounterID"), lit(62)),
                                   greater_equal(col("EventDate"), lit(parse_date("2013-07-14")))),
                          and_expr(less_equal(col("EventDate"), lit(parse_date("2013-07-15"))),
                                   equal(col("IsRefresh"), lit(0)))),
                 equal(col("DontCountHits"), lit(0)));

    auto minute_expr = date_trunc(col("EventTime"), DateTrunc::TruncSpec::minute);

    auto plan = q.top_k(
        q.group_by(
            q.filter(q.scan({"EventTime", "CounterID", "EventDate", "IsRefresh", "DontCountHits"}),
                     std::move(predicate)),
            make_groups(col("M", std::move(minute_expr))), make_aggs(count("*", "PageViews"))),
        top_k_specs(TopKOperator::OrderSpec{col("M"), TopKOperator::OrderDirection::Asc}),

        10, 1000);

    auto start = std::chrono::steady_clock::now();
    while (Batch* output_batch = plan->next()) {
        CsvWriter::write_batch(*output_batch);
    }
    auto end = std::chrono::steady_clock::now();

    std::cerr << "Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n\n";
}

void run_query(int n, const std::string& columnar_path) {
    switch (n) {
    case 0:
        query00(columnar_path);
        break;
    case 1:
        query01(columnar_path);
        break;
    case 2:
        query02(columnar_path);
        break;
    case 3:
        query03(columnar_path);
        break;
    case 4:
        query04(columnar_path);
        break;
    case 5:
        query05(columnar_path);
        break;
    case 6:
        query06(columnar_path);
        break;
    case 7:
        query07(columnar_path);
        break;
    case 8:
        query08(columnar_path);
        break;
    case 9:
        query09(columnar_path);
        break;

    case 10:
        query10(columnar_path);
        break;
    case 11:
        query11(columnar_path);
        break;
    case 12:
        query12(columnar_path);
        break;
    case 13:
        query13(columnar_path);
        break;
    case 14:
        query14(columnar_path);
        break;
    case 15:
        query15(columnar_path);
        break;
    case 16:
        query16(columnar_path);
        break;
    case 17:
        query17(columnar_path);
        break;
    case 18:
        query18(columnar_path);
        break;
    case 19:
        query19(columnar_path);
        break;
    case 20:
        query20(columnar_path);
        break;
    case 21:
        query21(columnar_path);
        break;
    case 22:
        query22(columnar_path);
        break;
    case 23:
        query23(columnar_path);
        break;
        // std::cerr << "Query 23 disabled (OOM risk)\n";
        // return;
    case 24:
        query24(columnar_path);
        break;
    case 25:
        query25(columnar_path);
        break;
    case 26:
        query26(columnar_path);
        break;
    case 27:
        query27(columnar_path);
        break;
    case 28:
        query28(columnar_path);
        break;
    case 29:
        query29(columnar_path);
        break;
    case 30:
        query30(columnar_path);
        break;
    case 31:
        query31(columnar_path);
        break;
    case 32:
        query32(columnar_path);
        break;
        // std::cerr << "Query 32 disabled (OOM risk)\n";
        // return;
    case 33:
        query33(columnar_path);
        break;
    case 34:
        query34(columnar_path);
        break;
    case 35:
        query35(columnar_path);
        break;
    case 36:
        query36(columnar_path);
        break;
    case 37:
        query37(columnar_path);
        break;
    case 38:
        query38(columnar_path);
        break;
    case 39:
        query39(columnar_path);
        break;
    case 40:
        query40(columnar_path);
        break;
    case 41:
        query41(columnar_path);
        break;
    case 42:
        query42(columnar_path);
        break;
    default:
        std::cerr << "Invalid query: " << n << '\n';
        break;
    }
}

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

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage:\n"
                      << "  --convert <schema_csv> <input_csv> <output_columnar>\n"
                      << "  --run-query <query_id> <columnar_path>\n";
            return 1;
        }

        std::string mode = argv[1];

        if (mode == "--convert") {
            if (argc != 5) {
                std::cerr << "Invalid --convert usage\n";
                return 1;
            }

            std::string csv_schema = argv[2];
            std::string csv_data = argv[3];
            std::string output_file = argv[4];

            convert(csv_schema, csv_data, output_file);

            return 0;
        }

        if (mode == "--run-query") {
            if (argc != 4) {
                std::cerr << "Invalid --run-query usage\n";
                return 1;
            }

            int query_id = std::stoi(argv[2]);
            std::string columnar_path = argv[3];

            run_query(query_id, columnar_path);

            return 0;
        }

        std::cerr << "Unknown mode: " << mode << "\n";

        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

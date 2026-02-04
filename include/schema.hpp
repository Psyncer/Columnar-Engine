#pragma once

#include "data_type.hpp"
// #include "parse_error.hpp"

#include <string>
#include <unordered_map>
#include <vector>
// #include <expected>

namespace columnar {

struct SchemaColumn {
    std::string name_;
    DataType type_;
};

class Schema {
public:
    size_t get_column_count() const;

    const SchemaColumn& get_column(size_t idx) const;

    size_t find_column_index(const std::string& name) const;
    
    void add_column(const std::string& name, DataType type);

private:
    std::vector<SchemaColumn> columns_;
    std::unordered_map<std::string, size_t> name_to_index_;
};

}  // namespace columnar

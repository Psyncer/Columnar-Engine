#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/storage/data_type.hpp"

namespace columnar {

struct SchemaColumn {
    std::string name;
    Type type;
};

class Schema {
private:
    std::vector<SchemaColumn> columns_;
    std::unordered_map<std::string, size_t> names_to_indices_;
    std::unordered_map<std::string, Type> names_to_types_;
    std::vector<std::string> names_;
    size_t column_count_ = 0;

public:
    size_t get_column_count() const;

    Type get_column_type(size_t idx) const;

    Type get_column_type(const std::string& name) const;

    const std::string& get_column_name(size_t idx) const;

    const std::vector<std::string>& get_column_names() const;

    size_t get_column_index(const std::string& name) const;

    bool contains(const std::string& name) const;

    void add_column(const std::string& name, Type type);  // throws
};

}  // namespace columnar

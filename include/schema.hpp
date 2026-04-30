#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "data_type.hpp"

namespace columnar {

struct SchemaColumn {
    std::string name_;
    Type type_;
};

class Schema {
private:
    std::vector<SchemaColumn> columns_;

public:
    size_t get_column_count() const;

    const SchemaColumn& get_column(size_t idx) const;

    void add_column(const std::string& name, Type type);  // throws
};

}  // namespace columnar

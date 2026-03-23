#pragma once

#include "data_type.hpp"
#include "parse_error.hpp"

#include <string>
#include <vector>
#include <expected>

namespace columnar {

struct SchemaColumn {
    std::string name_{};
    DataType type_{};
};

class Schema {
public:
    size_t get_column_count() const;

    Expected<const SchemaColumn&> get_column(size_t idx) const;

    
    void add_column(const std::string& name, DataType type);

private:
    std::vector<SchemaColumn> columns_{};
};

}  // namespace columnar

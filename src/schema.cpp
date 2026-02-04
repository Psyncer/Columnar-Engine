#include "schema.hpp"
#include "data_type.hpp"
// #include "parse_error.hpp"

#include <cstddef>
// #include <expected>
#include <functional>

namespace columnar {

size_t Schema::get_column_count() const {
    return columns_.size();
}

const SchemaColumn& Schema::get_column(size_t idx) const {
    return std::cref(columns_[idx]);
}

size_t Schema::find_column_index(const std::string& name) const {
    auto it = name_to_index_.find(name);

    return it->second;
}

void Schema::add_column(const std::string& name, DataType type) {
    size_t index = get_column_count();
    name_to_index_.emplace(name, index);
    columns_.emplace_back(name, type);
}

}  // namespace columnar

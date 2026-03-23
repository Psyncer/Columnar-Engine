#include "schema.hpp"
#include "data_type.hpp"
#include "parse_error.hpp"

#include <cstddef>
#include <expected>

namespace columnar {

size_t Schema::get_column_count() const {
    return columns_.size();
}

Expected<const SchemaColumn&> Schema::get_column(size_t idx) const {
    if (idx >= columns_.size()) {
        return std::unexpected(parse_error::index_out_of_range);
    }

    return columns_[idx];
}

void Schema::add_column(const std::string& name, DataType type) {
    columns_.emplace_back(name, type);
}

}  // namespace columnar

#include <cstddef>
#include <string>
#include <vector>

#include "assert.hpp"
#include "data_type.hpp"
#include "schema.hpp"

namespace columnar {

size_t Schema::get_column_count() const {
    return columns_.size();
}

const SchemaColumn& Schema::get_column(size_t idx) const {
    ASS(idx < columns_.size(), "index out of range");
    return columns_[idx];
}

void Schema::add_column(const std::string& name, Type type) {
    columns_.emplace_back(name, type);
}

}  // namespace columnar

#include <cstddef>
#include <string>
#include <vector>

#include "assert.hpp"
#include "data_type.hpp"
#include "schema.hpp"

namespace columnar {

size_t Schema::get_column_count() const {
    return column_count_;
}

Type Schema::get_column_type(size_t idx) const {
    ASS(idx < columns_.size(), "index out of range");
    return columns_[idx].type;
}

Type Schema::get_column_type(const std::string& name) const {
    if (name == "*") {
        return Type::Int64;
    }
    ASS(names_to_types_.contains(name), "no such name in schema");
    return names_to_types_.at(name);
}

const std::string& Schema::get_column_name(size_t idx) const {
    ASS(idx < columns_.size(), "index out of range");
    return columns_[idx].name;
}

const std::vector<std::string>& Schema::get_column_names() const {
    return names_;
}

size_t Schema::get_column_index(const std::string& name) const {
    if (name == "*" && !names_to_indices_.contains(name)) {
        return 0;
    }

    ASS(names_to_indices_.contains(name), "no such name in schema");
    return names_to_indices_.at(name);
}

bool Schema::contains(const std::string& name) const {
    return names_to_indices_.contains(name);
}

void Schema::add_column(const std::string& name, Type type) {
    columns_.emplace_back(name, type);
    names_to_indices_.emplace(name, column_count_);
    names_to_types_.emplace(name, type);
    names_.push_back(name);
    column_count_++;
}

}  // namespace columnar

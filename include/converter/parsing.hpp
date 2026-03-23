#pragma once

#include "data_type.hpp"
#include "parse_error.hpp"

#include <expected>
#include <string>
#include <vector>

namespace columnar {

Expected<std::vector<std::string>> split(const std::string& line, char delimiter);

Expected<DataType> get_data_type(const std::string& type);

Expected<int64_t> parse_int64(const std::string& token);

}  // namespace columnar

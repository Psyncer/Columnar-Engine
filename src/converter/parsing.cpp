#include "data_type.hpp"
#include "parse_error.hpp"

#include <expected>
#include <sstream>
#include <string>
#include <vector>

namespace columnar {

Expected<std::vector<std::string>> split(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    if (tokens.size() != 2) {
        return std::unexpected(parse_error::invalid_schema_format);
    }

    return tokens;
}

Expected<DataType> get_data_type(const std::string& type) {
    if (type == "int64") {
        return DataType::int64;
    }
    if (type == "string") {
        return DataType::string;
    }

    return std::unexpected(parse_error::unsupported_type);
}

Expected<int64_t> parse_int64(const std::string& token) {
    int64_t value;

    try {
        value = std::stol(token);
    } catch (const std::exception& e) {
        return std::unexpected(parse_error::bad_field);
    }

    return value;
}

}  // namespace columnar

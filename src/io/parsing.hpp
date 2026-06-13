#pragma once

#include <charconv>
#include <string>

#include "src/storage/data_type.hpp"

namespace columnar {

Type get_data_type(const std::string& type);

bool valid_date_format(const char* s);

bool valid_timestamp_format(const char* p);

template <typename T>
T parse_int(const std::string& token) {
    T value;

    const char* begin = token.data();
    const char* end = token.data() + token.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    ASS(ec == std::errc() && ptr == end, "not convertible to integer");

    return value;
}

std::string parse_string(const std::string& token);

int32_t parse_date(const std::string& token);

int64_t parse_timestamp(const std::string& token);

char parse_char(const std::string& token);

}  // namespace columnar

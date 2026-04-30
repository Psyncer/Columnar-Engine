#pragma once

#include <string>
#include <vector>

#include "data_type.hpp"

namespace columnar {

std::vector<std::string> split(const std::string& line, char delimiter);  // throws

Type get_data_type(const std::string& type);

bool valid_date_format(const char* s);

bool valid_timestamp_format(const char* p);

// template them

int16_t parse_int16(const std::string& token);

int32_t parse_int32(const std::string& token);

int64_t parse_int64(const std::string& token);

std::string parse_string(const std::string& token);

int32_t parse_date(const std::string& token);

int64_t parse_timestamp(const std::string& token);

char parse_char(const std::string& token);

}  // namespace columnar

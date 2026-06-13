#include <cstddef>
#include <string>

#include "tools/assert.hpp"
#include "src/storage/data_type.hpp"
#include "src/io/parsing.hpp"

namespace columnar {

Type get_data_type(const std::string& type) {
    if (type == "int16") {
        return Type::Int16;
    }
    if (type == "int32") {
        return Type::Int32;
    }
    if (type == "int64") {
        return Type::Int64;
    }
    if (type == "string") {
        return Type::String;
    }
    if (type == "DATE") {
        return Type::Date;
    }
    if (type == "TIMESTAMP") {
        return Type::Timestamp;
    }

    std::cerr << "Type not implemented" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
              << __func__ << std::endl;
    std::abort();
}

bool valid_date_format(const char* s) {
    return static_cast<unsigned>(s[0] - '0') <= 9 && static_cast<unsigned>(s[1] - '0') <= 9 &&
           static_cast<unsigned>(s[2] - '0') <= 9 && static_cast<unsigned>(s[3] - '0') <= 9 &&
           s[4] == '-' && static_cast<unsigned>(s[5] - '0') <= 9 &&
           static_cast<unsigned>(s[6] - '0') <= 9 && s[7] == '-' &&
           static_cast<unsigned>(s[8] - '0') <= 9 && static_cast<unsigned>(s[9] - '0') <= 9;
}

bool valid_timestamp_format(const char* p) {
    return valid_date_format(p) && p[10] == ' ' && static_cast<unsigned>(p[11] - '0') <= 9 &&
           static_cast<unsigned>(p[12] - '0') <= 9 && p[13] == ':' &&
           static_cast<unsigned>(p[14] - '0') <= 9 && static_cast<unsigned>(p[15] - '0') <= 9 &&
           p[16] == ':' && static_cast<unsigned>(p[17] - '0') <= 9 &&
           static_cast<unsigned>(p[18] - '0') <= 9;
}

int32_t parse_date(const std::string& token) {
    const char* p = token.data();
    size_t n = token.size();
    ASS(n >= 10, "incorrect date format");

    while (n > 0 && (*p == ' ' || *p == '\t')) {
        p++;
        n--;
    }

    ASS(valid_date_format(p), "incorrect date format");

    int y = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
    int m = (p[5] - '0') * 10 + (p[6] - '0');
    int d = (p[8] - '0') * 10 + (p[9] - '0');

    y -= static_cast<int>(m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return static_cast<int32_t>(era * 146097 + doe - 719468);
}

int64_t parse_timestamp(const std::string& token) {
    const char* p = token.data();
    size_t n = token.size();
    ASS(n >= 19, "incorrect timestamp format");

    while (n > 0 && (*p == ' ' || *p == '\t')) {
        p++;
        n--;
    }

    ASS(valid_timestamp_format(p), "incorrect timestamp format");

    int y = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
    int m = (p[5] - '0') * 10 + (p[6] - '0');
    int d = (p[8] - '0') * 10 + (p[9] - '0');
    int h = (p[11] - '0') * 10 + (p[12] - '0');
    int mi = (p[14] - '0') * 10 + (p[15] - '0');
    int s = (p[17] - '0') * 10 + (p[18] - '0');

    y -= static_cast<int>(m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int32_t days = static_cast<int32_t>(era * 146097 + doe - 719468);

    return static_cast<int64_t>(days) * 86400 + static_cast<int64_t>(h * 3600) +
           static_cast<int64_t>(mi * 60) + s;
}

char parse_char(const std::string& token) {
    ASS(token.size() == 1, "not a char");

    char c = token[0];

    return c;
}

}  // namespace columnar

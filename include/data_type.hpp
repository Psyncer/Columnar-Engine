#pragma once

#include <ostream>

namespace columnar {

enum struct DataType : int64_t {
    int64   = 0,
    string  = 1,
};

// для ручных тестов (потом убрать):
//
inline const char* to_string(DataType type) {
    switch (type) {
    case DataType::int64:
        return "int64";
    case DataType::string:
        return "string";
    }

    return "unsupported";
}

inline std::ostream& operator<<(std::ostream& os, DataType type) {
    return os << to_string(type);
}
//

}  // namespace columnar

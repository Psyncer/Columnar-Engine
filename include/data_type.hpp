#pragma once

#include <ostream>

namespace columnar {

enum class Type : int8_t {
    Int16       = 0,
    Int32       = 1,
    Int64       = 2,
    String      = 3,
    Date        = 4,
    Timestamp   = 5,
    Char        = 6,
};

inline const char* to_string(Type type) {
    switch (type) {
    case Type::Int16:
        return "int16";
    case Type::Int32:
        return "int32";
    case Type::Int64:
        return "int64";
    case Type::String:
        return "string";
    case Type::Date:
        return "DATE";
    case Type::Timestamp:
        return "TIMESTAMP";
    case Type::Char:
        return "char";
    default:
        return "UNSUPPORTED";
    }
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << to_string(type);
}

}  // namespace columnar

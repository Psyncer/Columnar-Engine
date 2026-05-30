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
};

template <typename T>
constexpr bool type_matches(Type type) {
    if constexpr (std::is_same_v<T, int16_t>) {
        return type == Type::Int16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return type == Type::Int32 || type == Type::Date;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return type == Type::Int64 || type == Type::Timestamp;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return type == Type::String;
    }

    return false;
}

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
    default:
        return "UNSUPPORTED";
    }
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << to_string(type);
}

}  // namespace columnar

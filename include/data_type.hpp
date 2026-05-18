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
struct TypeToEnum {
};

template <>
struct TypeToEnum<int16_t> {
    static constexpr Type value = Type::Int16;
};

template <>
struct TypeToEnum<int32_t> {
    static constexpr Type value = Type::Int32;
};

template <>
struct TypeToEnum<int64_t> {
    static constexpr Type value = Type::Int64;
};

template <>
struct TypeToEnum<std::string> {
    static constexpr Type value = Type::String;
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
    default:
        return "UNSUPPORTED";
    }
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << to_string(type);
}

}  // namespace columnar

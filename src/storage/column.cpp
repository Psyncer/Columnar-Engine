#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "src/storage/column.hpp"
#include "src/storage/data_type.hpp"
#include "tools/assert.hpp"

namespace columnar {

Column::Column() = default;

Column::Column(Type type) : type_(type) {
    switch (type_) {
    case Type::Int16:
        allocate<int16_t>();
        break;
    case Type::Int32:
        allocate<int32_t>();
        break;
    case Type::Int64:
        allocate<int64_t>();
        break;
    case Type::String:
        allocate_string();
        break;
    case Type::Date:
        allocate<int32_t>();
        break;
    case Type::Timestamp:
        allocate<int64_t>();
        break;
    }
}

Column::Column(Type type, int64_t literal) : type_(type) {
    switch (type_) {
    case Type::Int16: {
        allocate<int16_t>();
        int16_t* p = static_cast<int16_t*>(data_);
        for (size_t i = 0; i < kColumnCapacity; ++i) {
            p[i] = static_cast<int16_t>(literal);
            head_++;
        }
        break;
    }
    case Type::Int32: {
        allocate<int32_t>();
        int32_t* p = static_cast<int32_t*>(data_);
        for (size_t i = 0; i < kColumnCapacity; ++i) {
            p[i] = static_cast<int32_t>(literal);
            head_++;
        }
        break;
    }
    case Type::Int64: {
        allocate<int64_t>();
        int64_t* p = static_cast<int64_t*>(data_);
        for (size_t i = 0; i < kColumnCapacity; ++i) {
            p[i] = static_cast<int64_t>(literal);
            head_++;
        }
        break;
    }
    case Type::Date:
        allocate<int32_t>();
        {
            int32_t* p = static_cast<int32_t*>(data_);
            for (size_t i = 0; i < kColumnCapacity; ++i) {
                p[i] = static_cast<int32_t>(literal);
                head_++;
            }
            break;
        }
    case Type::Timestamp: {
        allocate<int64_t>();
        int64_t* p = static_cast<int64_t*>(data_);
        for (size_t i = 0; i < kColumnCapacity; ++i) {
            p[i] = static_cast<int64_t>(literal);
            head_++;
        }
        break;
    }
    default:
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
    }
}

Column::Column(Type type, std::string str) : type_(type) {
    allocate_string();
    char* p = static_cast<char*>(data_);
    int32_t pos = 0;
    while (str.size() * capacity_ >= kStringCapacity * capacity_) {
        reallocate_string();
    }
    for (size_t i = 0; i < capacity_; ++i) {
        offsets_.push_back(pos);
        std::memcpy(p + pos, str.data(), str.size());
        pos += static_cast<int32_t>(str.size());
        head_++;
    }
    offsets_.push_back(pos);
}

Column::Column(const Column& other)
    : capacity_(other.capacity_), head_(other.head_), type_(other.type_) {
    switch (type_) {
    case Type::Int16: {
        allocate<int16_t>();
        std::memcpy(data_, other.data_, head_ * sizeof(int16_t));
        break;
    }
    case Type::Int32: {
        allocate<int32_t>();
        std::memcpy(data_, other.data_, head_ * sizeof(int32_t));
        break;
    }
    case Type::Int64: {
        allocate<int64_t>();
        std::memcpy(data_, other.data_, head_ * sizeof(int64_t));
        break;
    }
    case Type::String: {
        allocate_string();
        size_t bytes = static_cast<size_t>(other.offsets_.back());
        std::memcpy(data_, other.data_, bytes);
        offsets_ = other.offsets_;
        break;
    }
    case Type::Date: {
        allocate<int32_t>();
        std::memcpy(data_, other.data_, head_ * sizeof(int32_t));
        break;
    }
    case Type::Timestamp: {
        allocate<int64_t>();
        std::memcpy(data_, other.data_, head_ * sizeof(int64_t));
        break;
    }
    }
}

Column::Column(Column&& other) noexcept {
    data_ = other.data_;
    offsets_ = other.offsets_;
    capacity_ = other.capacity_;
    head_ = other.head_;
    type_ = other.type_;

    other.data_ = nullptr;
    other.offsets_.clear();
    other.capacity_ = 0;
    other.head_ = 0;
}

Column::~Column() {
    std::free(data_);
}

Column& Column::operator=(Column&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::free(data_);

    data_ = other.data_;
    offsets_ = other.offsets_;
    capacity_ = other.capacity_;
    head_ = other.head_;
    type_ = other.type_;

    other.data_ = nullptr;
    other.offsets_.clear();
    other.capacity_ = 0;
    other.head_ = 0;

    return *this;
}

const void* Column::data() const {
    return data_;
}

void Column::push_string(const std::string& str) {
    while (static_cast<size_t>(offsets_.back()) + str.size() >= kStringCapacity * capacity_) {
        reallocate_string();
    }

    std::memcpy(static_cast<char*>(data_) + offsets_.back(), str.data(), str.size());
    head_++;
    offsets_.push_back(offsets_.back() + static_cast<int32_t>(str.size()));
}

void Column::emplace_string_column(const void* col, size_t size) {
    while (static_cast<size_t>(offsets_.back()) + size > kStringCapacity * capacity_) {
        reallocate_string();
    }

    const char* current = static_cast<const char*>(col);
    const char* end = current + size;

    while (current < end) {
        int32_t len;

        std::memcpy(&len, current, sizeof(int32_t));
        current += sizeof(int32_t);

        while (static_cast<size_t>(offsets_.back()) + static_cast<size_t>(len) >=
               kStringCapacity * capacity_) {
            reallocate_string();
        }

        std::memcpy(static_cast<char*>(data_) + offsets_.back(), current, static_cast<size_t>(len));
        head_++;
        offsets_.push_back(offsets_.back() + len);
        current += len;
    }
}

std::string Column::get_string(size_t idx) const {
    ASS(idx < head_, "index out of range");

    std::string str;
    size_t size = static_cast<size_t>(offsets_[idx + 1] - offsets_[idx]);
    str.resize(size);
    std::memcpy(str.data(), static_cast<char*>(data_) + offsets_[idx], size);

    return str;
}

std::string_view Column::get_string_view(size_t idx) const {
    ASS(idx < head_, "index out of range");

    std::string str;
    size_t size = static_cast<size_t>(offsets_[idx + 1] - offsets_[idx]);
    return std::string_view(static_cast<char*>(data_) + offsets_[idx], size);
}

size_t Column::size() const {
    return head_;
}

size_t Column::capacity() const {
    return capacity_;
}

Type Column::type() const {
    return type_;
}

void Column::clear() {
    head_ = 0;

    if (type_ == Type::String) {
        offsets_.clear();
        offsets_.push_back(0);
    }
}

void Column::allocate_string() {
    data_ = std::aligned_alloc(64, kStringCapacity * capacity_ * sizeof(char));
    offsets_.reserve(kColumnCapacity);
    offsets_.push_back(0);
}

void Column::reallocate_string() {
    capacity_ *= 2;
    void* new_data = std::aligned_alloc(64, kStringCapacity * capacity_ * sizeof(char));
    std::memcpy(new_data, data_, static_cast<size_t>(offsets_.back()));
    std::free(data_);
    data_ = new_data;
}

}  // namespace columnar

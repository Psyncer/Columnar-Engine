#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "assert.hpp"
#include "config.hpp"
#include "data_type.hpp"

namespace columnar {

class Column {
private:
    void* data_ = nullptr;
    std::vector<int32_t> offsets_;
    size_t capacity_ = kColumnCapacity;
    size_t head_ = 0;
    Type type_;
    size_t idx_;

public:
    Column(Type type, size_t idx) : type_(type), idx_(idx) {
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

    Column(Type type, size_t idx, int64_t literal) : type_(type), idx_(idx) {
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
            // error
            break;
        }
    }

    Column(Type type, size_t idx, std::string str) : type_(type), idx_(idx) {
        // ASS string type
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

    Column(const Column& other)
        : capacity_(other.capacity_), head_(other.head_), type_(other.type_), idx_(other.idx_) {
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

    Column(Column&& other) noexcept {
        data_ = other.data_;
        offsets_ = other.offsets_;
        capacity_ = other.capacity_;
        head_ = other.head_;
        type_ = other.type_;
        idx_ = other.idx_;

        other.data_ = nullptr;
        other.offsets_.clear();
        other.capacity_ = 0;
        other.head_ = 0;
    }

    ~Column() {
        std::free(data_);
    }

    Column& operator=(const Column& other) = delete;

    Column& operator=(Column&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        std::free(data_);

        data_ = other.data_;
        offsets_ = other.offsets_;
        capacity_ = other.capacity_;
        head_ = other.head_;
        type_ = other.type_;
        idx_ = other.idx_;

        other.data_ = nullptr;
        other.offsets_.clear();
        other.capacity_ = 0;
        other.head_ = 0;

        return *this;
    }

    const void* data() const {
        return data_;
    }

    template <typename T>
    const T* data_as() const {
        return static_cast<const T*>(data_);
    }

    template <typename T>
    void push(T value) {
        switch (type_) {
        case Type::Int16:
            push_value<int16_t>(static_cast<int16_t>(value));
            break;
        case Type::Int32:
            push_value<int32_t>(static_cast<int32_t>(value));
            break;
        case Type::Int64:
            push_value<int64_t>(static_cast<int64_t>(value));
            break;
        case Type::Date:
            push_value<int32_t>(static_cast<int32_t>(value));
            break;
        case Type::Timestamp:
            push_value<int64_t>(static_cast<int64_t>(value));
            break;
        default:
            // wrong type
            break;
        }
    }

    template <typename T>
    void push_value(T value) {
        ASS(type_matches<T>(type_), "wrong type");
        if (head_ + 1 >= capacity_) {
            reallocate<T>();
        }

        static_cast<T*>(data_)[head_] = value;
        head_++;
    }

    void push_string(const std::string& str) {
        while (static_cast<size_t>(offsets_.back()) + str.size() >= kStringCapacity * capacity_) {
            reallocate_string();
        }

        std::memcpy(static_cast<char*>(data_) + offsets_.back(), str.data(), str.size());
        head_++;
        offsets_.push_back(offsets_.back() + static_cast<int32_t>(str.size()));
    }

    template <typename T>
    void emplace_column(void* col, size_t size) {
        while (head_ + (size / sizeof(T)) > capacity_) {
            reallocate<T>();
        }

        std::memcpy(static_cast<T*>(data_) + head_, col, size);
        head_ += (size / sizeof(T));
    }

    void emplace_string_column(void* col, size_t size) {
        while (static_cast<size_t>(offsets_.back()) + size > kStringCapacity * capacity_) {
            reallocate_string();
        }

        const char* current = static_cast<char*>(col);
        const char* end = current + size;

        while (current < end) {
            int32_t len;

            std::memcpy(&len, current, sizeof(int32_t));
            current += sizeof(int32_t);

            while (static_cast<size_t>(offsets_.back()) + static_cast<size_t>(len) >=
                   kStringCapacity * capacity_) {
                reallocate_string();
            }

            std::memcpy(static_cast<char*>(data_) + offsets_.back(), current,
                        static_cast<size_t>(len));
            head_++;
            offsets_.push_back(offsets_.back() + len);
            current += len;
        }
    }

    template <typename T>
    T get_value(size_t idx) const {
        return static_cast<T*>(data_)[idx];
    }

    std::string get_string(size_t idx) const {
        ASS(idx < head_, "index out of range");

        std::string str;
        size_t size = static_cast<size_t>(offsets_[idx + 1] - offsets_[idx]);
        str.resize(size);
        std::memcpy(str.data(), static_cast<char*>(data_) + offsets_[idx], size);

        return str;
    }

    size_t size() const {
        return head_;
    }

    Type type() const {
        return type_;
    }

    size_t index() const {
        return idx_;
    }

    void clear() {
        head_ = 0;

        if (type_ == Type::String) {
            offsets_.clear();
            offsets_.push_back(0);
        }
    }

private:
    template <typename T>
    void allocate() {
        data_ = std::aligned_alloc(64, capacity_ * sizeof(T));
    }

    void allocate_string() {
        data_ = std::aligned_alloc(64, kStringCapacity * capacity_ * sizeof(char));
        offsets_.reserve(kColumnCapacity);
        offsets_.push_back(0);
    }

    template <typename T>
    void reallocate() {
        capacity_ *= 2;
        void* new_data = std::aligned_alloc(64, capacity_ * sizeof(T));
        std::memcpy(new_data, data_, head_ * sizeof(T));
        std::free(data_);
        data_ = new_data;
    }

    void reallocate_string() {
        capacity_ *= 2;
        void* new_data = std::aligned_alloc(64, kStringCapacity * capacity_ * sizeof(char));
        std::memcpy(new_data, data_, static_cast<size_t>(offsets_.back()));
        std::free(data_);
        data_ = new_data;
    }
};

}  // namespace columnar

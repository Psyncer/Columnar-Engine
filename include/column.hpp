#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "config.hpp"
#include "data_type.hpp"

namespace columnar {

class Column {
private:
    void* data_ = nullptr;
    int32_t* offsets_ = nullptr;
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

    ~Column() {
        std::free(data_);
        std::free(offsets_);
    }

    Column(const Column&) = delete;

    Column& operator=(const Column&) = delete;

    Column(Column&& other) noexcept {
        data_ = other.data_;
        offsets_ = other.offsets_;
        capacity_ = other.capacity_;
        head_ = other.head_;
        type_ = other.type_;
        idx_ = other.idx_;

        other.data_ = nullptr;
        other.offsets_ = nullptr;
        other.capacity_ = 0;
        other.head_ = 0;
    }

    Column& operator=(Column&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        std::free(data_);
        std::free(offsets_);

        data_ = other.data_;
        offsets_ = other.offsets_;
        capacity_ = other.capacity_;
        head_ = other.head_;
        type_ = other.type_;
        idx_ = other.idx_;

        other.data_ = nullptr;
        other.offsets_ = nullptr;
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

    size_t string_offset(size_t idx) const {
        return static_cast<size_t>(offsets_[idx]);
    }

    size_t string_len(size_t idx) const {
        return static_cast<size_t>(offsets_[idx] - offsets_[idx - 1]);
    }

    template <typename T>
    void push_value(T value) {
        if (head_ + 1 >= capacity_) {
            reallocate<T>();
        }

        static_cast<T*>(data_)[head_] = value;
        head_++;
    }

    void push_string(const std::string& str) {
        if (static_cast<size_t>(offsets_[head_]) + str.size() >= kStringCapacity * capacity_) {
            reallocate_string();
        }

        std::memcpy(static_cast<char*>(data_) + offsets_[head_], str.data(), str.size());
        head_++;
        offsets_[head_] = offsets_[head_ - 1] + static_cast<int32_t>(str.size());
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
        while (static_cast<size_t>(offsets_[head_]) + size > kStringCapacity * capacity_) {
            reallocate_string();
        }

        const char* current = static_cast<char*>(col);
        const char* end = current + size;

        while (current < end) {
            int32_t len;

            std::memcpy(&len, current, sizeof(int32_t));
            current += sizeof(int32_t);

            while (static_cast<size_t>(offsets_[head_]) + static_cast<size_t>(len) >=
                   kStringCapacity * capacity_) {
                reallocate_string();
            }

            std::memcpy(static_cast<char*>(data_) + offsets_[head_], current,
                        static_cast<size_t>(len));
            head_++;
            offsets_[head_] = offsets_[head_ - 1] + len;

            current += len;
        }
    }

    template <typename T>
    T get_value(size_t idx) const {
        return static_cast<T*>(data_)[idx];
    }

    std::string get_string(size_t idx) const {
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
    }

private:
    template <typename T>
    void allocate() {
        data_ = std::aligned_alloc(64, capacity_ * sizeof(T));
    }

    void allocate_string() {
        data_ = std::aligned_alloc(64, kStringCapacity * capacity_ * sizeof(char));
        offsets_ =
            static_cast<int32_t*>(std::aligned_alloc(64, (capacity_ + 64) * sizeof(int32_t)));
        offsets_[0] = 0;
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
        std::memcpy(new_data, data_, static_cast<size_t>(offsets_[head_]));
        std::free(data_);
        data_ = new_data;
    }
};

}  // namespace columnar

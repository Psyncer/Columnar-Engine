#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "tools/assert.hpp"
#include "tools/config.hpp"
#include "src/storage/data_type.hpp"

namespace columnar {

class Column {
private:
    void* data_ = nullptr;
    std::vector<int64_t> offsets_;
    size_t capacity_ = kColumnCapacity;
    size_t head_ = 0;
    Type type_;

public:
    Column();

    Column(Type type);

    Column(Type type, int64_t literal);

    Column(Type type, std::string str);

    Column(const Column& other);

    Column(Column&& other) noexcept;

    ~Column();

    Column& operator=(const Column& other) = delete;

    Column& operator=(Column&& other) noexcept;

    const void* data() const;

    template <typename T>
    const T* data_as() const {
        return static_cast<const T*>(data_);
    }

    template <typename T, typename Integral>
    void push_value(Integral value) {
        ASS(type_matches<T>(type_), "wrong type");
        if (head_ + 1 >= capacity_) {
            reallocate<T>();
        }

        static_cast<T*>(data_)[head_] = static_cast<T>(value);
        head_++;
    }

    void push_string(const std::string& str);

    template <typename T>
    void emplace_column(const void* col, size_t size) {
        while (head_ + (size / sizeof(T)) > capacity_) {
            reallocate<T>();
        }

        std::memcpy(static_cast<T*>(data_) + head_, col, size);
        head_ += (size / sizeof(T));
    }

    void emplace_string_column(const void* col, size_t size);

    template <typename T>
    T get_value(size_t idx) const {
        return static_cast<T*>(data_)[idx];
    }

    std::string get_string(size_t idx) const;

    std::string_view get_string_view(size_t idx) const;

    size_t size() const;

    size_t capacity() const;

    Type type() const;

    void clear();

private:
    template <typename T>
    void allocate() {
        data_ = std::aligned_alloc(64, capacity_ * sizeof(T));
    }

    void allocate_string();

    template <typename T>
    void reallocate() {
        capacity_ *= 2;
        void* new_data = std::aligned_alloc(64, capacity_ * sizeof(T));
        std::memcpy(new_data, data_, head_ * sizeof(T));
        std::free(data_);
        data_ = new_data;
    }

    void reallocate_string();
};

}  // namespace columnar

#pragma once

#include <cstddef>

namespace columnar {

inline constexpr size_t kReadBufSize = 1ULL << 20;     // 1 MB
inline constexpr size_t kBufThreshold = 1ULL << 14;    // 16 KB
inline constexpr size_t kReadSchemaSize = 1ULL << 13;  // 8 KB

inline constexpr size_t kWriteBufSize = 1ULL << 23;    // 8 MB

inline constexpr size_t kTempBufferSize = 1ULL << 21;  // 2 MB

inline constexpr size_t kColumnCapacity = 4096;
inline constexpr size_t kStringCapacity = 128;

inline constexpr size_t kConversionBatchCapacity = 4096;

}  // namespace columnar

#ifndef BM_TYPES_HPP
#define BM_TYPES_HPP

#include <cinttypes>
#include <filesystem>
#include <numeric>

namespace bm {

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8  = std::uint8_t;

using i64 = std::int64_t;
using i32 = std::int32_t;
using i16 = std::int16_t;
using i8  = std::int8_t;

using f32 = float;
using f64 = double;

template<typename T>
struct Pixel {
    T r;
    T g;
    T b;
};

template<typename T>
using lim = std::numeric_limits<T>;

namespace fs = std::filesystem;

}

#endif
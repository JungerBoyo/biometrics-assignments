#ifndef BM_HISTOGRAM_HPP
#define BM_HISTOGRAM_HPP

#include <array>
#include <span>

#include "Types.hpp"

namespace bm {

struct Histogram {
    std::array<u32, 256> mean_sums;
    std::array<u32, 256> r_sums;
    std::array<u32, 256> g_sums;
    std::array<u32, 256> b_sums;
    std::size_t full_sum{ 0 };

    void clear();
    void set(const u8* data, std::size_t len, std::size_t channels_num);

    enum class Channel { ALL, R, G, B };

    void computeDistributantForChannel(std::array<f32, 256>& result, Channel channel) const;
    void normalizeForChannel(std::array<f32, 256>& result, Channel channel) const;
};

}

#endif

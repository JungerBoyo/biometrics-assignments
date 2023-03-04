#include "Histogram.hpp"

using namespace bm;

void Histogram::clear() {
    std::fill(mean_sums.begin(), mean_sums.end(), 0);
    std::fill(r_sums.begin(), r_sums.end(), 0);
    std::fill(g_sums.begin(), g_sums.end(), 0);
    std::fill(b_sums.begin(), b_sums.end(), 0);
}

void Histogram::set(const u8* data, std::size_t len, std::size_t channels_num) {
    const auto data_span = std::span<const u8>(data, len);
    for (std::size_t i{ 0 }; i<data_span.size(); i+=channels_num) {
        const auto r = data_span[i + 0];
        const auto g = data_span[i + 1];
        const auto b = data_span[i + 2];

        ++r_sums[r];                
        ++g_sums[g];                
        ++b_sums[b];       
        ++mean_sums[static_cast<std::size_t>(r + g + b) / 3UL];
    }
}
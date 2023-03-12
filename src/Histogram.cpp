#include "Histogram.hpp"

using namespace bm;

void Histogram::clear() {
    std::fill(mean_sums.begin(), mean_sums.end(), 0);
    std::fill(r_sums.begin(), r_sums.end(), 0);
    std::fill(g_sums.begin(), g_sums.end(), 0);
    std::fill(b_sums.begin(), b_sums.end(), 0);
}

void Histogram::set(const u8* data, std::size_t len, std::size_t channels_num) {
    full_sum = len / channels_num;

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

void Histogram::computeDistributantForChannel(std::array<f32, 256>& result, Channel channel) const {
    const auto overall_count_f = static_cast<f32>(full_sum);

    const std::array<u32, 256>* sums = nullptr;
    switch (channel) {
    case Channel::R: sums = &r_sums; break;
    case Channel::G: sums = &g_sums; break;        
    case Channel::B: sums = &b_sums; break;        
    case Channel::ALL: sums = &mean_sums; break;        
    }

    u32 sum{ 0 };
    for (std::size_t i{ 0 }; i < result.size(); ++i) {
        sum += (*sums)[i];
        result[i] = static_cast<f32>(sum) / overall_count_f;
    }
}

void Histogram::normalizeForChannel(std::array<f32, 256>& result, Channel channel) const {
    const auto overall_count_f = static_cast<f32>(full_sum);

    const std::array<u32, 256>* sums = nullptr;
    switch (channel) {
    case Channel::R: sums = &r_sums; break;
    case Channel::G: sums = &g_sums; break;        
    case Channel::B: sums = &b_sums; break;        
    case Channel::ALL: sums = &mean_sums; break;        
    }

    for (std::size_t i{ 0 }; i < result.size(); ++i) {
        result[i] = static_cast<f32>((*sums)[i]) / overall_count_f;
    }
}
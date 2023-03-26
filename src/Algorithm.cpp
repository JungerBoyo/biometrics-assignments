#include "Algorithm.hpp"

#include <algorithm>
#include <fstream>
#include <charconv>

#include <spdlog/spdlog.h>
#include <glad/glad.h>

using namespace bm;

void ThresholdBinarizationAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(ThresholdBinarizationDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}
void ThresholdBinarizationAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(ThresholdBinarizationDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}

void OtsuBinarizationAlgorithm::prepare(const Histogram& histogram) {
    std::array<f32, 256> normalized_mean_histogram;
    histogram.normalizeForChannel(normalized_mean_histogram, Histogram::Channel::ALL);
    
    std::array<f32, 256> sums;
    std::array<f32, 256> means;
    sums[0] = normalized_mean_histogram[0];
    means[0] = 0.F;
    for (std::size_t i{ 1 }; i < normalized_mean_histogram.size(); ++i) {
        sums[i] = sums[i-1] + normalized_mean_histogram[i];
        means[i] = means[i-1] + normalized_mean_histogram[i] * static_cast<f32>(i);
    }

    std::array<f32, 256> per_threshold_variance;
    for (std::size_t i{ 0 }; i < per_threshold_variance.size(); ++i) {
        const f32 weight0 = sums[i];
        const f32 weight1 = 1.F - weight0;
        const f32 mean0 = means[i] / weight0;
        const f32 mean1 = (means[255] - means[i]) / weight1;
        per_threshold_variance[i] = weight0 * weight1 * (mean0 - mean1) * (mean0 - mean1);
    }

    const auto max_variance = std::max_element(per_threshold_variance.cbegin(), per_threshold_variance.cend());
    const auto threshold = std::distance(per_threshold_variance.cbegin(), max_variance);

    this->descriptor.threshold = static_cast<f32>(threshold) / 255.F;
}
void OtsuBinarizationAlgorithm::continuousSubmit(u32 buff_id) {}
void OtsuBinarizationAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(OtsuBinarizationDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}

void EqualizationAlgorithm::prepare(const Histogram& histogram) {
    histogram.computeDistributantForChannel(descriptor.distributant_r, Histogram::Channel::R);
    histogram.computeDistributantForChannel(descriptor.distributant_g, Histogram::Channel::G);
    histogram.computeDistributantForChannel(descriptor.distributant_b, Histogram::Channel::B);

    const auto is_positive = [](f32 value) { return value > 0.F; };

    if (const auto found = std::find_if(
            descriptor.distributant_r.cbegin(), 
            descriptor.distributant_r.cend(),
            is_positive); 
        found != descriptor.distributant_r.cend()) {
        descriptor.distributant_r0 = *found;
    } else {
        descriptor.distributant_r0 = 0;
    }
    if (const auto found = std::find_if(
            descriptor.distributant_g.cbegin(), 
            descriptor.distributant_g.cend(),
            is_positive); 
        found != descriptor.distributant_g.cend()) {
        descriptor.distributant_g0 = *found;
    } else {
        descriptor.distributant_g0 = 0;
    }
    if (const auto found = std::find_if(
            descriptor.distributant_b.cbegin(), 
            descriptor.distributant_b.cend(),
            is_positive); 
        found != descriptor.distributant_b.cend()) {
        descriptor.distributant_b0 = *found;
    } else {
        descriptor.distributant_b0 = 0;
    }
}
void EqualizationAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(i32), 
        static_cast<const void*>(&descriptor.range)
    );
}
void EqualizationAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(EqualizationDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}

void StretchingAlgorithm::prepare(const Image& image) {
    const auto[min, max] = image.minMax<f32>();
    descriptor.local_min[0] = min.r;
    descriptor.local_min[1] = min.g;
    descriptor.local_min[2] = min.b;

    descriptor.local_max[0] = max.r;
    descriptor.local_max[1] = max.g;
    descriptor.local_max[2] = max.b;
}
void StretchingAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(StretchingDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}
void StretchingAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id, 
        0, sizeof(StretchingDescriptor), 
        static_cast<const void*>(&descriptor)
    );
}

void LocalBinarizationAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(LocalBinarizationDescriptor),
        static_cast<const void*>(&descriptor)
    );
}
void LocalBinarizationAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(LocalBinarizationDescriptor),
        static_cast<const void*>(&descriptor)
    );
}

void ConvolutionAlgorithm::prepare(const fs::path& filter_path) {
    const auto filter_path_str = filter_path.string();

    std::ifstream stream(filter_path);
    if (!stream.good()) {
        spdlog::error("Couldn't read filter from {}", filter_path_str);
        return;
    }

    std::string line;
    std::getline(stream, line);

    if (const auto result = std::from_chars(&line.front(), &line.back() + 1, descriptor.kernel_size);
        result.ec != std::errc()) {
        spdlog::error("Failed to parse kernel size from {}", filter_path_str);
        return;
    }
    if (descriptor.kernel_size < 0 || descriptor.kernel_size > 10) {
        spdlog::error("Kernel size declared in {} isn't in range [{}, {}], is {}", 
            filter_path_str,
            0, 10, 
            descriptor.kernel_size
        );
        return;
    }

    const auto kernel_side_size = static_cast<std::size_t>(2 * descriptor.kernel_size + 1);

    i32 line_num{ 0 };
    for (std::size_t line_num{ 0 }; std::getline(stream, line); ++line_num) {
        std::from_chars_result result{};
        result.ptr = &line.front() - 1;
        for (std::size_t i{0}; i<kernel_side_size; ++i) {
            f32 value{ 0.F };
            result = std::from_chars(result.ptr + 1, &line.back() + 1, value);
            if (result.ec != std::errc()) {
                spdlog::error("Failed to parse kernel value from {}", filter_path_str);
                return;
            }
            descriptor.kernel[i + line_num * kernel_side_size] = value;
        }
    }
    stream.close();

    float kernel_elements_sum{0.F};
    for (std::size_t i{0}; i < kernel_side_size*kernel_side_size; ++i) {
        kernel_elements_sum += descriptor.kernel[i];
    }
    if (kernel_elements_sum > 0.F) {
        for (std::size_t i{0}; i < kernel_side_size*kernel_side_size; ++i) {
            descriptor.kernel[i] /= kernel_elements_sum;
        }
    }
}
void ConvolutionAlgorithm::continuousSubmit(u32 buff_id) {}
void ConvolutionAlgorithm::submit(u32 buff_id) {
    const auto kernel_size_in_bytes = sizeof(f32) *
        (2 * descriptor.kernel_size + 1) * 
        (2 * descriptor.kernel_size + 1);
    glNamedBufferSubData(
        buff_id,
        0, offsetof(ConvolutionDescriptor, kernel) + kernel_size_in_bytes,
        static_cast<const void*>(&descriptor)
    );
}

void MedianFilterAlgorithm::prepare() {}
void MedianFilterAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(MedianFilterDescriptor),
        static_cast<const void*>(&descriptor)
    );
}
void MedianFilterAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(MedianFilterDescriptor),
        static_cast<const void*>(&descriptor)
    );
}


void PixelizationAlgorithm::prepare(u32 tex_id, u32 binding) {
    glBindImageTexture(binding, tex_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
}
void PixelizationAlgorithm::continuousSubmit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(PixelizationDescriptor),
        static_cast<const void*>(&descriptor)
    );
}
void PixelizationAlgorithm::submit(u32 buff_id) {
    glNamedBufferSubData(
        buff_id,
        0, sizeof(PixelizationDescriptor),
        static_cast<const void*>(&descriptor)
    );
}

#ifndef BM_ALGORITHM_HPP
#define BM_ALGORITHM_HPP

#include <algorithm>

#include "Shader.hpp"
#include "Image.hpp"
#include "Shader.hpp"
#include "Histogram.hpp"
#include "Types.hpp"

namespace bm {

template <typename DescriptorType, typename... DependencyArgs>
struct Algorithm {
	using Base = Algorithm<DescriptorType, DependencyArgs...>; 

	DescriptorType descriptor;
	const Shader &shader;

	Algorithm(const Shader &shader) : shader(shader) {}

	virtual void prepare(DependencyArgs... args) = 0;
	virtual void continuousSubmit(u32 buff_id) = 0;
	virtual void submit(u32 buff_id) = 0;

	virtual ~Algorithm() = default;
};

struct ThresholdBinarizationDescriptor {
	enum : i32 {
		BINARIZE_CHANNEL_ALL,
		BINARIZE_CHANNEL_R,
		BINARIZE_CHANNEL_G,
		BINARIZE_CHANNEL_B
	};
	f32 threshold = .0F;
	i32 channel = BINARIZE_CHANNEL_ALL;
};
struct ThresholdBinarizationAlgorithm : Algorithm<ThresholdBinarizationDescriptor> {
	ThresholdBinarizationAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare() override {}
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~ThresholdBinarizationAlgorithm() override = default;
};

struct OtsuBinarizationDescriptor {
	f32 threshold = .0F;
};
struct OtsuBinarizationAlgorithm : Algorithm<OtsuBinarizationDescriptor, const Histogram &> {
	OtsuBinarizationAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare(const Histogram &) override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~OtsuBinarizationAlgorithm() override = default;
};

struct EqualizationDescriptor {
	i32 range = 256;
	f32 distributant_r0{0.F};
	f32 distributant_g0{0.F};
	f32 distributant_b0{0.F};
	std::array<f32, 256> distributant_r;
	std::array<f32, 256> distributant_g;
	std::array<f32, 256> distributant_b;
};
struct EqualizationAlgorithm : Algorithm<EqualizationDescriptor, const Histogram &> {
	EqualizationAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare(const Histogram &histogram) override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~EqualizationAlgorithm() override = default;
};

struct StretchingDescriptor {
	alignas(16) f32 local_min[3] = {0.F, 0.F, 0.F};
	alignas(16) f32 local_max[3] = {1.F, 1.F, 1.F};
	alignas(16) f32 global_max[3] = {1.F, 1.F, 1.F};
};
struct StretchingAlgorithm : Algorithm<StretchingDescriptor, const Image &> {
	StretchingAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare(const Image &image) override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~StretchingAlgorithm() override = default;
};

struct LocalBinarizationDescriptor {
	enum : i32 { NIBLACK_EQUATION, SAVOULA_EQUATION, PHANSCALAR_EQUATION };

	i32 kernel_size{1};
	i32 equation_type{NIBLACK_EQUATION};
	f32 ratio{.5F};
	f32 standard_deviation_div{2.F};
	f32 pow{2.F};
	f32 q{10.F};
};
struct LocalBinarizationAlgorithm : Algorithm<LocalBinarizationDescriptor> {
	LocalBinarizationAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare() override {}
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~LocalBinarizationAlgorithm() override = default;
};

struct ConvolutionDescriptor {
	i32 kernel_size{1}; // max 10
	alignas(4) bool gray_scale{false};
	alignas(4) bool gradient{false};
	alignas(16) std::array<f32, 441> kernel;
};
struct ConvolutionAlgorithm : Algorithm<ConvolutionDescriptor, const fs::path &> {
	ConvolutionAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare(const fs::path &filter_path) override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~ConvolutionAlgorithm() override = default;
};

struct MedianFilterDescriptor {
	int kernel_size{1};
};
struct MedianFilterAlgorithm : Algorithm<MedianFilterDescriptor> {
	MedianFilterAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare() override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~MedianFilterAlgorithm() override = default;
};

struct PixelizationDescriptor {
	int kernel_size{1};
};
struct PixelizationAlgorithm : Algorithm<PixelizationDescriptor, u32, u32> {
	PixelizationAlgorithm(const Shader &shader) : Base(shader) {}

	void prepare(u32 tex_id, u32 binding_id) override;
	void continuousSubmit(u32 buff_id) override;
	void submit(u32 buff_id) override;

	~PixelizationAlgorithm() override = default;
};

} // namespace bm

#endif
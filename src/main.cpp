#include <array>
#include <bitset>
#include <optional>
#include <ranges>
#include <span>

#include <Algorithm.hpp>
#include <DirManager.hpp>
#include <Framebuffer.hpp>
#include <Histogram.hpp>
#include <Image.hpp>
#include <Quad.hpp>
#include <Shader.hpp>
#include <Texture2D.hpp>
#include <Window.hpp>
#include <config.hpp>

#include <glad/glad.h>

#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <imgui.h>
#include <imgui_bindings/imgui_impl_glfw.h>
#include <imgui_bindings/imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <implot.h>

using namespace bm;

constexpr i64 MAX_ALG_DESCRIPTOR_SIZE{4096};

constexpr std::string_view DEFAULT_ASSET_IMAGE_PATH{"assets/textures/Bikesgray.jpg"};

namespace ImGui {

#define IMGUI_DISABLED(x)\
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);\
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);\
	x;\
	ImGui::PopItemFlag();\
	ImGui::PopStyleVar()

#define IMGUI_DISABLED_CONDITION(c, x)\
	if (c) {\
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);\
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);\
		x ImGui::PopItemFlag();\
		ImGui::PopStyleVar();\
	} else {\
		x\
	}

#define IMGUI_DISABLED_RAW(x)\
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);\
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);\
	x ImGui::PopItemFlag();\
	ImGui::PopStyleVar()

struct SelectablePathList {
	int selected{0};
	bool refresh_state{false};

	bool draw(const std::vector<fs::path> &paths, const char *list_name) {
		bool result = false;
		if (ImGui::IsWindowFocused()) {
			if (!refresh_state) {
				refresh_state = true;
				result = true;
			}
		} else {
			refresh_state = false;
		}

		if (ImGui::BeginListBox(list_name)) {
			i32 i{0};
			for (const auto &path : paths) {
				const auto is_selected = i == selected;
				if (ImGui::Selectable(path.filename().c_str(), is_selected)) {
					selected = i;
				}
				++i;

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndListBox();
		}

		return result;
	}
};

}

struct WindowData {
	// moving and scaling data
	f32 &quad_x_offset;
	f32 &quad_y_offset;
	f32 &quad_scale;
	bool lhs_button_pressed_mv{false};
	f32 last_mouse_x_pos{0.F};
	f32 last_mouse_y_pos{0.F};
	i32 window_width{0};
	i32 window_height{0};
	// pencil drawing data
	bool& draw_mode;
	bool& lhs_button_pressed_draw;
};

static void mousePositionCallback(GLFWwindow *win_handle, double x_pos, double y_pos);
static void mouseButtonCallback(GLFWwindow *win_handle, int button, int action, int mods);
static void mouseScrollCallback(GLFWwindow *win_handle, double xoffset, double yoffset);
static void keyCallback(GLFWwindow* win_handle, int key, int scancode, int action, int mods);

int main() {
	// intialize glfw3 + glad
	Window window(
		cmake::project_name, 640, 480,
		[](int err_code, const char *message) {
			spdlog::error("[GLFW3]{}:{}", err_code, message);
		},
		[](u32, u32, u32 id, u32, int, const char *message, const void *) {
			spdlog::error("[OPENGL]{}:{}", id, message);
		}
	);
	// initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow *>(window.native()), true);
	ImGui_ImplOpenGL3_Init("#version 450");

	// App data
	struct TransformData {
		f32 quad_scale{.5F};
		f32 quad_x_offset{0.F};
		f32 quad_y_offset{0.F};
		f32 aspect_ratio{1.F};
		u32 flip_tex_y_axis_xor{0};
		alignas(16) f32 drawing_cursor_color[4];
	} transform_data;
	std::array<f32, 3> clear_color{{0.F, 0.F, 0.F}};


	window.setMousePositionCallback(mousePositionCallback);
	window.setMouseButtonCallback(mouseButtonCallback);
	window.setMouseScrollCallback(mouseScrollCallback);
	window.setKeyCallback(keyCallback);

	u32 quad_vbo_id{0U};
	u32 quad_vao_id{0U};
	u32 quad_ubo_id{0U};
	u32 alg_descriptor_ubo_id{0U};

	Shader basic_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/basic_shader/vert.spv",
		"shaders/bin/basic_shader/frag.spv",
	});
	Shader threshold_binarization_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/threshold_binarization_shader/vert.spv",
		"shaders/bin/threshold_binarization_shader/frag.spv",
	});
	Shader otsu_binarization_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/otsu_binarization_shader/vert.spv",
		"shaders/bin/otsu_binarization_shader/frag.spv",
	});
	Shader stretching_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/stretching_shader/vert.spv",
		"shaders/bin/stretching_shader/frag.spv",
	});
	Shader equalization_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/equalization_shader/vert.spv",
		"shaders/bin/equalization_shader/frag.spv",
	});
	Shader local_binarization_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/local_binarization_shader/vert.spv",
		"shaders/bin/local_binarization_shader/frag.spv"
	});
	Shader convolution_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/convolution_shader/vert.spv",
		"shaders/bin/convolution_shader/frag.spv"
	});
	Shader median_filter_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/median_filter_shader/vert.spv",
		"shaders/bin/median_filter_shader/frag.spv"
	});
	Shader pixelization_shader(Shader::Type::COMPUTE, {"shaders/bin/pixelization_shader/comp.spv"});

	Shader drawing_cursor_shader(Shader::Type::VERTEX_FRAGMENT,
	{
		"shaders/bin/drawing_cursor_shader/vert.spv",
		"shaders/bin/drawing_cursor_shader/frag.spv"
	});

	// create VBO and initialize
	glCreateBuffers(1, &quad_vbo_id);
	glCreateVertexArrays(1, &quad_vao_id);

	initQuad(quad_vbo_id, quad_vao_id);

	// create UBOs
	glCreateBuffers(1, &quad_ubo_id);
	glCreateBuffers(1, &alg_descriptor_ubo_id);
	glNamedBufferStorage(
		quad_ubo_id, sizeof(transform_data),
		static_cast<const void *>(&transform_data),
		GL_DYNAMIC_STORAGE_BIT
	);
	glNamedBufferStorage(
		alg_descriptor_ubo_id, MAX_ALG_DESCRIPTOR_SIZE, nullptr,
		GL_DYNAMIC_STORAGE_BIT
	);
	glBindBufferBase(
		GL_UNIFORM_BUFFER, SHCONFIG_TRANSFORM_UBO_BINDING,
		quad_ubo_id
	);
	glBindBufferBase(
		GL_UNIFORM_BUFFER, SHCONFIG_ALG_DESCRIPTOR_UBO_BINDING,
		alg_descriptor_ubo_id
	);
	// create Texture and load image
	Image image(DEFAULT_ASSET_IMAGE_PATH);
	Texture2D img_texture({
		.width = image.width,
		.height = image.height,
		.internal_fmt = GL_RGBA8,
		.fmt = GL_RGBA,
		.type = GL_UNSIGNED_BYTE,
		.wrap_s = GL_REPEAT,
		.wrap_t = GL_REPEAT,
		.min_filter = GL_NEAREST,
		.mag_filter = GL_NEAREST,
		.mipmap = false
	});
	img_texture.update(static_cast<const void *>(image.pixels.data()));
	img_texture.bind(SHCONFIG_2D_TEX_BINDING);

	// Create texture FBO
	FBO fbo({
		.width = image.width,
		.height = image.height,
		.color_attachment = GL_COLOR_ATTACHMENT0,
		.internal_fmt = GL_RGBA8
	});
	// bind
	basic_shader.bind();
	glBindVertexArray(quad_vao_id);

	// Histogram
	Histogram histogram;
	histogram.clear();
	histogram.set(
		image.pixels.data(), image.pixels.size(),
		static_cast<std::size_t>(image.channels_num)
	);

	// App algorithms
	ThresholdBinarizationAlgorithm threshold_binarization_alg(threshold_binarization_shader);
	OtsuBinarizationAlgorithm otsu_binarization_alg(otsu_binarization_shader);
	EqualizationAlgorithm equalization_alg(equalization_shader);
	StretchingAlgorithm stretching_alg(stretching_shader);
	LocalBinarizationAlgorithm local_binarization_alg(local_binarization_shader);
	ConvolutionAlgorithm convolution_alg(convolution_shader);
	MedianFilterAlgorithm median_filter_alg(median_filter_shader);
	PixelizationAlgorithm pixelization_alg(pixelization_shader);

	const auto alg_perform_fn = [&] {
		fbo.bind();
		glViewport(0, 0, image.width, image.height);
		TransformData tmp_transform_data{.quad_scale = 1.F, .flip_tex_y_axis_xor = 1};
		glNamedBufferSubData(
			quad_ubo_id, 0, sizeof(tmp_transform_data),
			static_cast<const void *>(&tmp_transform_data)
		);
		glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

		glReadPixels(
			0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE,
			static_cast<void *>(image.pixels.data())
		);
		glCopyTextureSubImage2D(
			img_texture.tex_id_, 0, 0, 0, 0, 0, 
			image.width, image.height
		);

		fbo.unbind();
	};

	const std::function<void()> submit_binarization_data_fn = [&] {
		threshold_binarization_alg.continuousSubmit(alg_descriptor_ubo_id);
	};
	const std::function<void()> submit_stretching_data_fn = [&] {
		stretching_alg.continuousSubmit(alg_descriptor_ubo_id);
	};
	const std::function<void()> submit_equalization_data_fn = [&] {
		equalization_alg.continuousSubmit(alg_descriptor_ubo_id);
	};
	const std::function<void()> submit_local_binarization_data_fn = [&] {
		local_binarization_alg.continuousSubmit(alg_descriptor_ubo_id);
	};
	const std::function<void()> *submit_current_alg_data_fn = nullptr;

	// dir managers
	DirManager assets_dir_manager("assets/textures", {".png", ".jpg", "jpeg"});
	DirManager filters_dir_manager("assets/filters", {".ftr"});

	// imgui selectable lists
	ImGui::SelectablePathList selectable_assets_list;
	ImGui::SelectablePathList selectable_filter_list;

	// window visibility logic artifacts
	constexpr std::size_t WINDOWS_COUNT{12};
	enum WIN_TYPE : std::size_t {
		THRESHOLD_BINARIZATION,
		LOCAL_BINARIZATION,
		OTSU_BINARIZATION,
		STRETCHING,
		EQUALIZATION,
		CONVOLUTION,
		MEDIAN_FILTER,
		PIXELIZATION,
		IMAGE_DATA_DETAILS,
		CONFIG,
		ASSETS,
		DRAWING,
	};
	std::bitset<WINDOWS_COUNT> win_visibility_mask(lim<std::size_t>::max());

	// toolbox
	struct DrawingDescriptor {
	    i32 width_px{ 1 };
	    f32 color[4] = { 0.F, 0.F, 0.F, 1.F };
		bool draw_mode{ false };
		bool drawing{ false };
	};
	DrawingDescriptor drawing_descriptor{};

	// Window data	
	WindowData window_data{
		.quad_x_offset = transform_data.quad_x_offset,
		.quad_y_offset = transform_data.quad_y_offset,
		.quad_scale = transform_data.quad_scale,
		
		.draw_mode = drawing_descriptor.draw_mode,
		.lhs_button_pressed_draw = drawing_descriptor.drawing
	};
	window.setWinUserDataPointer(static_cast<void *>(&window_data));

	// main loop
	while (!window.shouldClose()) {
		window.pollEvents();

		const auto [width, height] = window.size();
		glViewport(0, 0, width, height);
		window_data.window_width = width;
		window_data.window_height = height;
		glClearColor(clear_color[0], clear_color[1], clear_color[2], 1.F);

		transform_data.aspect_ratio =
			(static_cast<f32>(width) / static_cast<f32>(height)) *
			(static_cast<f32>(image.height) / static_cast<f32>(image.width));

		glNamedBufferSubData(
			quad_ubo_id, 0, sizeof(transform_data),
			static_cast<const void *>(&transform_data)
		);

		if (submit_current_alg_data_fn != nullptr) {
			(*submit_current_alg_data_fn)();
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// if (win_visibility_mask[WIN_TYPE::CONFIG]) {
		// 	ImGui::Begin("Config");
		// 	ImGui::SliderFloat("scaling", &transform_data.quad_scale, 0.F, 1.F);
		// 	ImGui::Separator();
		// 	ImGui::ColorPicker3("clear color", clear_color.data());
		// 	ImGui::End();
		// }

		if (win_visibility_mask[WIN_TYPE::THRESHOLD_BINARIZATION]) {
			ImGui::Begin("Threshold binarization");
			ImGui::SliderFloat("threshold", &threshold_binarization_alg.descriptor.threshold, 0.F, 1.F);
			{
				static u8 states = 0x01;
				if (ImGui::RadioButton("binarize all channels", (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL)) > 0)) {
					threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL;
					states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL);
				}
				if (ImGui::RadioButton("binarize red channel", (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R)) > 0)) {
					threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R;
					states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R);
				}
				if (ImGui::RadioButton("binarize green channel", (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G)) > 0)) {
					threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G;
					states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G);
				}
				if (ImGui::RadioButton("binarize blue channel", (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_B)) > 0)) {
					threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_B;
					states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_B);
				}
			}
			if (ImGui::Button("Perform single##0")) {
				threshold_binarization_alg.submit(alg_descriptor_ubo_id);
				threshold_binarization_alg.shader.bind();
				alg_perform_fn();
				basic_shader.bind();
			}
			if (static bool state = false; ImGui::Checkbox("Perform continuously##0", &state)) {
				if (state) {
					win_visibility_mask.reset();
					win_visibility_mask.set(WIN_TYPE::THRESHOLD_BINARIZATION);

					submit_current_alg_data_fn = &submit_binarization_data_fn;
					threshold_binarization_alg.shader.bind();
				} else {
					win_visibility_mask.set();

					threshold_binarization_alg.submit(alg_descriptor_ubo_id);
					threshold_binarization_alg.shader.bind();
					alg_perform_fn();
					basic_shader.bind();
					submit_current_alg_data_fn = nullptr;
				}
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::OTSU_BINARIZATION]) {
			ImGui::Begin("Otsu binarization");
			IMGUI_DISABLED(ImGui::SliderFloat("threshold", &otsu_binarization_alg.descriptor.threshold, 0.F, 1.F));
			if (ImGui::Button("Perform")) {
				histogram.clear();
				histogram.set(image.pixels.data(), image.pixels.size(), static_cast<std::size_t>(image.channels_num));
				otsu_binarization_alg.prepare(histogram);
				otsu_binarization_alg.submit(alg_descriptor_ubo_id);
				otsu_binarization_alg.shader.bind();
				alg_perform_fn();
				basic_shader.bind();
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::STRETCHING]) {
			ImGui::Begin("Stretching");
			ImGui::SliderFloat3("global max", stretching_alg.descriptor.global_max, 0.F, 1.F);
			if (ImGui::Button("Perform single##1")) {
				stretching_alg.prepare(image);
				stretching_alg.submit(alg_descriptor_ubo_id);

				stretching_alg.shader.bind();
				alg_perform_fn();
				basic_shader.bind();
			}
			if (static bool state = false; ImGui::Checkbox("Perform continuously##1", &state)) {
				if (state) {
					win_visibility_mask.reset();
					win_visibility_mask.set(WIN_TYPE::STRETCHING);

					stretching_alg.prepare(image);
					submit_current_alg_data_fn = &submit_stretching_data_fn;
					stretching_shader.bind();
				} else {
					win_visibility_mask.set();

					stretching_alg.submit(alg_descriptor_ubo_id);
					stretching_alg.shader.bind();
					alg_perform_fn();
					basic_shader.bind();
					submit_current_alg_data_fn = nullptr;
				}
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::EQUALIZATION]) {
			ImGui::Begin("Equalization");
			ImGui::SliderInt("range", &equalization_alg.descriptor.range, 1, 256);
			if (ImGui::Button("Perform single##2")) {
				histogram.clear();
				histogram.set(image.pixels.data(), image.pixels.size(), static_cast<std::size_t>(image.channels_num));
				equalization_alg.prepare(histogram);
				equalization_alg.submit(alg_descriptor_ubo_id);

				equalization_alg.shader.bind();

				alg_perform_fn();

				basic_shader.bind();
			}
			if (static bool state = false; ImGui::Checkbox("Perform continuously##2", &state)) {
				if (state) {
					win_visibility_mask.reset();
					win_visibility_mask.set(WIN_TYPE::EQUALIZATION);

					histogram.clear();
					histogram.set(image.pixels.data(), image.pixels.size(), static_cast<std::size_t>(image.channels_num));
					equalization_alg.prepare(histogram);
					equalization_alg.submit(alg_descriptor_ubo_id);

					equalization_alg.shader.bind();

					submit_current_alg_data_fn = &submit_equalization_data_fn;
				} else {
					win_visibility_mask.set();

					equalization_alg.shader.bind();
					alg_perform_fn();
					basic_shader.bind();
					submit_current_alg_data_fn = nullptr;
				}
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::LOCAL_BINARIZATION]) {
			ImGui::Begin("Local binarization");
			ImGui::SliderInt("Kernel size", &local_binarization_alg.descriptor.kernel_size, 1, 20);
			{
				ImGui::RadioButton("Niblack",
					&local_binarization_alg.descriptor.equation_type,
					LocalBinarizationDescriptor::NIBLACK_EQUATION
				);
				ImGui::SameLine();
				ImGui::RadioButton("Savoula",
					&local_binarization_alg.descriptor.equation_type,
					LocalBinarizationDescriptor::SAVOULA_EQUATION
				);
				ImGui::SameLine();
				ImGui::RadioButton("Phanscalar",
					&local_binarization_alg.descriptor.equation_type,
					LocalBinarizationDescriptor::PHANSCALAR_EQUATION
				);

				ImGui::SliderFloat("Ratio", &local_binarization_alg.descriptor.ratio, 0.F, 1.F);

				const auto equation_type = local_binarization_alg.descriptor.equation_type;
				if (equation_type == LocalBinarizationDescriptor::NIBLACK_EQUATION) {
					IMGUI_DISABLED(ImGui::SliderFloat(
						"Standard deviation div",
						&local_binarization_alg.descriptor.standard_deviation_div, 0.F,
						10.F)
					);
				} else {
					ImGui::SliderFloat(
						"Standard deviation div",
						&local_binarization_alg.descriptor.standard_deviation_div, 0.F,
						10.F
					);
				}

				if (equation_type < LocalBinarizationDescriptor::PHANSCALAR_EQUATION) {
					IMGUI_DISABLED(ImGui::SliderFloat("pow", &local_binarization_alg.descriptor.pow, 0.F, 10.F));
					IMGUI_DISABLED(ImGui::SliderFloat("q", &local_binarization_alg.descriptor.q, 0.F, 10.F));
				} else {
					ImGui::SliderFloat("pow", &local_binarization_alg.descriptor.pow, 0.F, 10.F);
					ImGui::SliderFloat("q", &local_binarization_alg.descriptor.q, 0.F, 10.F);
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Perform single##3")) {
				local_binarization_alg.submit(alg_descriptor_ubo_id);
				local_binarization_alg.shader.bind();

				alg_perform_fn();

				basic_shader.bind();
			}
			if (static bool state = false; ImGui::Checkbox("Perform continuously##3", &state)) {
				if (state) {
					win_visibility_mask.reset();
					win_visibility_mask.set(WIN_TYPE::LOCAL_BINARIZATION);

					local_binarization_alg.submit(alg_descriptor_ubo_id);
					local_binarization_alg.shader.bind();

					submit_current_alg_data_fn = &submit_local_binarization_data_fn;
				} else {
					win_visibility_mask.set();

					local_binarization_alg.shader.bind();
					alg_perform_fn();
					basic_shader.bind();
					submit_current_alg_data_fn = nullptr;
				}
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::CONVOLUTION]) {
			ImGui::Begin("Convolution");
			IMGUI_DISABLED(ImGui::SliderInt("Kernel size", &convolution_alg.descriptor.kernel_size, 0, 10));
			ImGui::Checkbox("Grayscale", &convolution_alg.descriptor.gray_scale);
			ImGui::Checkbox("Gradient", &convolution_alg.descriptor.gradient);
			if (selectable_filter_list.draw(filters_dir_manager.files, "Available filters")) {
				filters_dir_manager.refresh();
			}
			if (ImGui::Button("Convolve using selected filter")) {
				const auto filter_path_index = static_cast<std::size_t>(selectable_filter_list.selected);
				convolution_alg.prepare(filters_dir_manager.files[filter_path_index]);
				convolution_alg.submit(alg_descriptor_ubo_id);
				convolution_alg.shader.bind();

				alg_perform_fn();

				basic_shader.bind();
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::MEDIAN_FILTER]) {
			ImGui::Begin("Median filter");
			ImGui::SliderInt("Kernel size", &median_filter_alg.descriptor.kernel_size, 1, 3);
			if (ImGui::Button("Perform single##4")) {
				median_filter_alg.submit(alg_descriptor_ubo_id);
				median_filter_alg.shader.bind();

				alg_perform_fn();

				basic_shader.bind();
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::PIXELIZATION]) {
			ImGui::Begin("Pixelization");
			ImGui::SliderInt("Kernel size", &pixelization_alg.descriptor.kernel_size, 2, 100);
			if (ImGui::Button("Perform single##4")) {
				pixelization_alg.prepare(img_texture.tex_id_, SHCONFIG_COMPUTE_IMAGE_BINDING);
				pixelization_alg.submit(alg_descriptor_ubo_id);
				pixelization_alg.shader.bind();

				const auto kernel_size = pixelization_alg.descriptor.kernel_size;

				const auto num_groups_x = 
					static_cast<u32>(image.width / kernel_size) +
					((static_cast<u32>(image.width) % static_cast<u32>(kernel_size)) != 0 ? 1U : 0U);
					
				const auto num_groups_y = 
					static_cast<u32>(image.height / kernel_size) +
					((static_cast<u32>(image.height) % static_cast<u32>(kernel_size)) != 0 ? 1U : 0U);

				glDispatchCompute(num_groups_x, num_groups_y, 1);
				glTextureBarrier();

				basic_shader.bind();
			}

			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::IMAGE_DATA_DETAILS]) {
			ImGui::Begin("Image data details");
			if (ImPlot::BeginPlot("Histogram", "Possible pixel values", "Count of pixel values in image", ImVec2{-1.F, -1.F})) {
				ImPlot::SetNextFillStyle({.8F, .8F, .8F, 1.F}); // gray
				ImPlot::PlotBars("From rgb mean", histogram.mean_sums.data(), static_cast<int>(histogram.mean_sums.size()));
				ImPlot::SetNextFillStyle({.8F, .0F, .0F, 1.F}); // red
				ImPlot::PlotBars("From red channel", histogram.r_sums.data(), static_cast<int>(histogram.r_sums.size()));
				ImPlot::SetNextFillStyle({.0F, .8F, .0F, 1.F}); // green
				ImPlot::PlotBars("From green channel", histogram.g_sums.data(), static_cast<int>(histogram.g_sums.size()));
				ImPlot::SetNextFillStyle({.0F, .0F, .8F, 1.F}); // blue
				ImPlot::PlotBars("From blue channel", histogram.b_sums.data(), static_cast<int>(histogram.b_sums.size()));

				ImPlot::EndPlot();
			}
			if (ImGui::Button("Update", ImVec2{-1.F, 0.F})) {
				histogram.clear();
				histogram.set(
					image.pixels.data(), image.pixels.size(),
					static_cast<std::size_t>(image.channels_num)
				);
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::ASSETS]) {
			ImGui::Begin("Assets");
			if (selectable_assets_list.draw(assets_dir_manager.files, "Available images")) {
				assets_dir_manager.refresh();
			}
			if (ImGui::Button("Load selected image")) {
				const auto asset_index = static_cast<std::size_t>(selectable_assets_list.selected);
				if (image.update(assets_dir_manager.files.at(asset_index))) {
					img_texture.unbind(SHCONFIG_2D_TEX_BINDING);
					img_texture.resize(image.width, image.height);
					img_texture.update(static_cast<const void *>(image.pixels.data()));
					img_texture.bind(SHCONFIG_2D_TEX_BINDING);

					fbo.resize(image.width, image.height);
				}
			}
			if (ImGui::Button("Save to selected image")) {
				const auto asset_index = static_cast<std::size_t>(selectable_assets_list.selected);
				image.save(assets_dir_manager.files.at(asset_index));
			}
			ImGui::End();
		}

		if (win_visibility_mask[WIN_TYPE::DRAWING] && ImGui::Begin("Drawing")) {
			ImGui::SliderInt("Pencil width [px]", &drawing_descriptor.width_px, 1, 128);
			ImGui::ColorPicker4("Color", drawing_descriptor.color);
			ImGui::End();
		}

		if (drawing_descriptor.draw_mode) {
			TransformData tmp{};
			const auto px_size_window = 1.F/static_cast<f32>(window_data.window_width);
			const auto image_width_in_window_px = transform_data.quad_scale * static_cast<f32>(window_data.window_width);
			const auto window_image_px_ratio = image_width_in_window_px/static_cast<f32>(image.width);
			const auto px_scale_image = px_size_window * window_image_px_ratio; 
			const auto px_size_image = 2.F * px_scale_image; // window bounds are (-1.0, 1.0)

			tmp.quad_scale = static_cast<f32>(drawing_descriptor.width_px) * px_scale_image;
			tmp.aspect_ratio = (static_cast<f32>(window_data.window_width) / static_cast<f32>(window_data.window_height));

			const auto image_offset_correction_x = px_size_image * (
				(transform_data.quad_scale * transform_data.quad_x_offset)/px_size_image -
				std::roundf((transform_data.quad_scale * transform_data.quad_x_offset)/px_size_image));
			tmp.quad_x_offset =
				image_offset_correction_x +
				px_size_image *
				std::roundf((2.F * (window_data.last_mouse_x_pos/window_data.window_width) - 1.F) / px_size_image) +
				((drawing_descriptor.width_px % 2 == 1) ? px_scale_image : 0.F);

			const auto logical_px_scale_image_in_y = tmp.aspect_ratio * px_scale_image;
			const auto logical_px_size_image_in_y = 2.F * logical_px_scale_image_in_y;

			const auto image_offset_correction_y = logical_px_size_image_in_y * (
				(transform_data.quad_scale * transform_data.quad_y_offset)/logical_px_size_image_in_y -
				std::roundf((transform_data.quad_scale * transform_data.quad_y_offset)/logical_px_size_image_in_y));
			tmp.quad_y_offset =
				image_offset_correction_y +
				-logical_px_size_image_in_y *
				std::roundf((2.F * (window_data.last_mouse_y_pos/window_data.window_height) - 1.F) / logical_px_size_image_in_y) -
				((drawing_descriptor.width_px % 2 == 1) ? logical_px_scale_image_in_y : 0.F);

			std::memcpy(static_cast<void*>(tmp.drawing_cursor_color), static_cast<void*>(drawing_descriptor.color), 4 * sizeof(f32));

			if (drawing_descriptor.drawing) {
				//quad_x_offset in image space
				tmp.quad_x_offset = tmp.quad_x_offset / transform_data.quad_scale - transform_data.quad_x_offset;
				//quad_y_offset in image space
				const auto logical_scale_in_y = transform_data.quad_scale * transform_data.aspect_ratio;
				tmp.quad_y_offset = -(tmp.quad_y_offset / logical_scale_in_y - transform_data.quad_y_offset/transform_data.aspect_ratio);
				tmp.aspect_ratio = static_cast<f32>(image.width) / static_cast<f32>(image.height);
				tmp.quad_scale = (1.F / static_cast<f32>(image.width)) * static_cast<f32>(drawing_descriptor.width_px);

				drawing_cursor_shader.bind();

				glNamedBufferSubData(
					quad_ubo_id, 0, sizeof(tmp),
					static_cast<const void *>(&tmp)
				);

				img_texture.unbind(SHCONFIG_2D_TEX_BINDING);
				fbo.bind();

				glViewport(0, 0, image.width, image.height);
				glFramebufferTexture2D(
					GL_FRAMEBUFFER,
					GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_2D, img_texture.tex_id_, 0
				);
				
				glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

				glTextureBarrier();

				glFramebufferTexture2D(
					GL_FRAMEBUFFER,
					GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_2D, fbo.tex_id_, 0
				);

				fbo.unbind();

				img_texture.bind(SHCONFIG_2D_TEX_BINDING);

				basic_shader.bind();	
			} else {
				drawing_cursor_shader.bind();	
				
				glNamedBufferSubData(
					quad_ubo_id, 0, sizeof(tmp),
					static_cast<const void *>(&tmp)
				);
	
				glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

				basic_shader.bind();	
			}
		}

		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.swapBuffers();
	}
	// opengl stuff
	std::array<u32, 3> buffers{{quad_vbo_id, quad_ubo_id, alg_descriptor_ubo_id}};
	glDeleteBuffers(buffers.size(), buffers.data());
	glDeleteVertexArrays(1, &quad_vao_id);
	std::array<u32, 2> textures{{img_texture.tex_id_, fbo.tex_id_}};
	glDeleteTextures(textures.size(), textures.data());
	glDeleteFramebuffers(1, &fbo.fbo_id_);
	basic_shader.deinit();
	threshold_binarization_shader.deinit();
	otsu_binarization_shader.deinit();
	stretching_shader.deinit();
	equalization_shader.deinit();
	local_binarization_shader.deinit();
	convolution_shader.deinit();
	median_filter_shader.deinit();
	pixelization_shader.deinit();
	// imgui stuff
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	// window
	window.deinit();
}

void mousePositionCallback(GLFWwindow* win_handle, double x_pos, double y_pos) {
	auto *window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(win_handle));

	if (window_data->lhs_button_pressed_mv) {
		static constexpr f32 OFFSET_WAGE{1.5F};
		const auto dx = static_cast<f32>(x_pos) - window_data->last_mouse_x_pos;
		const auto dy = static_cast<f32>(y_pos) - window_data->last_mouse_y_pos;

		const auto scale_power = 1.F / window_data->quad_scale;

		window_data->quad_x_offset += scale_power * dx * OFFSET_WAGE / static_cast<f32>(window_data->window_width);	
		window_data->quad_y_offset -= scale_power * dy * OFFSET_WAGE / static_cast<f32>(window_data->window_height);
	}

	window_data->last_mouse_x_pos = static_cast<f32>(x_pos);
	window_data->last_mouse_y_pos = static_cast<f32>(y_pos);
}
void mouseButtonCallback(GLFWwindow* win_handle, int button, int action, int mods) {
	const bool is_mod_ctrl = ((mods & GLFW_MOD_CONTROL) > 0 && (mods & ~GLFW_MOD_CONTROL) == 0);
	auto *window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(win_handle));

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if ((action & (GLFW_PRESS|GLFW_REPEAT)) > 0) {
			if (!window_data->lhs_button_pressed_mv && !window_data->lhs_button_pressed_draw) {
				if (window_data->draw_mode) {
					window_data->lhs_button_pressed_draw = true;
				} else {
					window_data->lhs_button_pressed_mv = is_mod_ctrl;
				}
			}
		} else if (action == GLFW_RELEASE) {
			window_data->lhs_button_pressed_mv = false;
			window_data->lhs_button_pressed_draw = false;
		}
	}
}
void mouseScrollCallback(GLFWwindow* win_handle, double, double yoffset) {
	auto* window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(win_handle));

	static constexpr f32 SCALE_WAGE{0.05F};

	window_data->quad_scale += window_data->quad_scale * SCALE_WAGE * static_cast<f32>(yoffset);
}

void keyCallback(GLFWwindow* win_handle, int key, int scancode, int action, int mods) {
	auto* window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(win_handle));

	if (key == GLFW_KEY_D && (mods & (GLFW_MOD_CONTROL|GLFW_MOD_SHIFT)) > 0 && action == GLFW_PRESS) {
		window_data->draw_mode = !window_data->draw_mode;
	}
}
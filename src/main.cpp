#include <array>
#include <span>
#include <optional>
#include <ranges>

#include <Shader.hpp>
#include <Window.hpp>
#include <config.hpp>
#include <Quad.hpp>
#include <Histogram.hpp>
#include <Image.hpp>
#include <Framebuffer.hpp>
#include <Texture2D.hpp>
#include <Algorithm.hpp>

#include <glad/glad.h>

#include <stb_image.h>
#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_bindings/imgui_impl_glfw.h>
#include <imgui_bindings/imgui_impl_opengl3.h>

#include <implot.h>

using namespace bm;

// using u64 = std::uint64_t;
// using u32 = std::uint32_t;
// using u16 = std::uint16_t;
// using u8  = std::uint8_t;

// using i64 = std::int64_t;
// using i32 = std::int32_t;
// using i16 = std::int16_t;
// using i8  = std::int8_t;

// using f32 = float;
// using f64 = double;

constexpr i64 MAX_ALG_DESCRIPTOR_SIZE{4096};

constexpr std::string_view DEFAULT_ASSET_IMAGE_PATH {"assets/textures/OLFJ3yF.jpg"};
constexpr std::string_view ASSETS_DIR_RELATIVE_PATH {"assets/textures"};
constexpr std::array<std::string_view, 3> ACCEPTED_ASSETS_EXTENSIONS {
    ".png",
    ".jpg",
    ".jpeg"
};

std::optional<std::vector<fs::path>> findFilesWithExtensionsInDir(
    const fs::path& dir_path, 
    std::span<const std::string_view> extensions
) {
    if (!fs::is_directory(dir_path)) {
        return std::nullopt;
    }

    std::vector<fs::path> result;

    for (const auto& entry : fs::directory_iterator{dir_path}) {
        if (entry.is_regular_file()) {
            auto extension = entry.path().extension().string();
            std::transform(
                extension.begin(), 
                extension.end(), 
                extension.begin(), 
                [](u8 c) { return std::tolower(c); }
            );
            
            if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                result.emplace_back(std::move(entry.path()));                
            }
        }
    }

    return result;
}

int main() {
    // intialize glfw3 + glad
    Window window(cmake::project_name, 640, 480, 
    [](int err_code, const char* message){
        spdlog::error("[GLFW3]{}:{}", err_code, message);
    },[](u32, u32, u32 id, u32, int, const char *message, const void *){
        spdlog::error("[OPENGL]{}:{}", id, message);
    });
    // initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window.native()), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // App data
    struct TransformData {
        float quad_scale{ .5F };
        float aspect_ratio{ 1.F };
        u32 flip_tex_y_axis_xor{ 0 };
    } transform_data;
    std::array<float, 3> clear_color{{0.F, 0.F, 0.F}};
    
    u32 quad_vbo_id{ 0U };
    u32 quad_vao_id{ 0U };
    u32 quad_ubo_id{ 0U };
    u32 alg_descriptor_ubo_id{ 0U };

    Shader basic_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/basic_shader/vert.spv",
            "shaders/bin/basic_shader/frag.spv",
        }
    );
    Shader threshold_binarization_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/threshold_binarization_shader/vert.spv",
            "shaders/bin/threshold_binarization_shader/frag.spv",
        }
    );
    Shader otsu_binarization_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/otsu_binarization_shader/vert.spv",
            "shaders/bin/otsu_binarization_shader/frag.spv",
        }
    );
    Shader stretching_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/stretching_shader/vert.spv",
            "shaders/bin/stretching_shader/frag.spv",
        }
    );
    Shader equalization_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/equalization_shader/vert.spv",
            "shaders/bin/equalization_shader/frag.spv",
        }
    );

    // create VBO and initialize
    glCreateBuffers(1, &quad_vbo_id);
    glCreateVertexArrays(1, &quad_vao_id);

    initQuad(quad_vbo_id, quad_vao_id);

    // create UBOs 
    glCreateBuffers(1, &quad_ubo_id);
    glCreateBuffers(1, &alg_descriptor_ubo_id);
    glNamedBufferStorage(
        quad_ubo_id, 
        sizeof(transform_data), 
        static_cast<const void*>(&transform_data), 
        GL_DYNAMIC_STORAGE_BIT
    );
    glNamedBufferStorage(
        alg_descriptor_ubo_id, 
        MAX_ALG_DESCRIPTOR_SIZE, 
        nullptr,
        GL_DYNAMIC_STORAGE_BIT
    );
    glBindBufferBase(GL_UNIFORM_BUFFER, SHCONFIG_TRANSFORM_UBO_BINDING, quad_ubo_id);
    glBindBufferBase(GL_UNIFORM_BUFFER, SHCONFIG_ALG_DESCRIPTOR_UBO_BINDING, alg_descriptor_ubo_id);
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
        .min_filter = 0,
        .mag_filter = 0,
        .mipmap = false
    });
    img_texture.update(static_cast<const void*>(image.pixels.data()));
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

    // Asset img list
    i32 selected_asset{ 0 };
    auto asset_imgs_paths = findFilesWithExtensionsInDir(
        ASSETS_DIR_RELATIVE_PATH, 
        std::span(ACCEPTED_ASSETS_EXTENSIONS)
    );   
    if (!asset_imgs_paths.has_value()) {
        spdlog::error("Failed to list asset files from {}", ASSETS_DIR_RELATIVE_PATH);
        return 1;
    }

    // Histogram
    Histogram histogram;
    histogram.clear();
    histogram.set(image.pixels.data(), image.pixels.size(), static_cast<std::size_t>(image.channels_num));

    // App algorithms 
    ThresholdBinarizationAlgorithm threshold_binarization_alg(threshold_binarization_shader);
    OtsuBinarizationAlgorithm otsu_binarization_alg(otsu_binarization_shader);
    EqualizationAlgorithm equalization_alg(equalization_shader);
    StretchingAlgorithm stretching_alg(stretching_shader);

    const auto alg_perform_fn = [&]{
        fbo.bind();
        glViewport(0, 0, image.width, image.height);
        TransformData tmp_transform_data { .quad_scale = 1.F, .flip_tex_y_axis_xor = 1};
        glNamedBufferSubData(
            quad_ubo_id, 
            0, sizeof(tmp_transform_data), 
            static_cast<const void*>(&tmp_transform_data)
        );
        glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

        glReadPixels(
            0, 0, image.width, image.height, 
            GL_RGBA, GL_UNSIGNED_BYTE,
            static_cast<void*>(image.pixels.data())
        );
        glCopyTextureSubImage2D(img_texture.tex_id_, 0, 0, 0, 0, 0, image.width, image.height);

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
    const std::function<void()>* submit_current_alg_data_fn = nullptr;

    // main loop
    while (!window.shouldClose()) {
        window.pollEvents();

        const auto[width, height] = window.size();
        glViewport(0, 0, width, height);
        glClearColor(
            clear_color[0], 
            clear_color[1],
            clear_color[2],
            1.F
        );

        transform_data.aspect_ratio = 
            (static_cast<float>(width) /
            static_cast<float>(height)) *
            (static_cast<float>(image.height) /
            static_cast<float>(image.width));
        
        glNamedBufferSubData(
            quad_ubo_id, 
            0, sizeof(transform_data), 
            static_cast<const void*>(&transform_data)
        );

        if (submit_current_alg_data_fn != nullptr) {
            (*submit_current_alg_data_fn)();
        }

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTICES.size());

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Config");
            ImGui::SliderFloat("scaling", &transform_data.quad_scale, 0.F, 1.F);
            ImGui::Separator();
            ImGui::ColorPicker3("clear color", clear_color.data());
        ImGui::End();

        ImGui::Begin("Threshold binarization");
            ImGui::SliderFloat(
                "threshold", 
                &threshold_binarization_alg.descriptor.threshold,
                0.F,
                1.F
            );
            {
                static u8 states = 0x01;                
                if (ImGui::RadioButton(
                        "binarize all channels", 
                        (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL)) > 0
                    )) {
                    threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL;
                    states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_ALL);
                }
                if (ImGui::RadioButton(
                        "binarize red channel", 
                        (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R)) > 0
                    )) {
                    threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R;
                    states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_R);
                }
                if (ImGui::RadioButton(
                        "binarize green channel", 
                        (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G)) > 0
                    )) {
                    threshold_binarization_alg.descriptor.channel = ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G;
                    states = (states & 0x00) | (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_G);
                }
                if (ImGui::RadioButton(
                        "binarize blue channel", 
                        (states & (0x01 << ThresholdBinarizationDescriptor::BINARIZE_CHANNEL_B)) > 0
                    )) {
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
                    submit_current_alg_data_fn = &submit_binarization_data_fn;
                    threshold_binarization_alg.shader.bind();
                } else {
                    threshold_binarization_alg.submit(alg_descriptor_ubo_id);
                    threshold_binarization_alg.shader.bind();
                    alg_perform_fn();
                    basic_shader.bind();
                    submit_current_alg_data_fn = nullptr;
                }
            }
        ImGui::End();

        ImGui::Begin("Otsu binarization");
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);
                ImGui::SliderFloat("threshold", &otsu_binarization_alg.descriptor.threshold, 0.F, 1.F);
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
            if (ImGui::Button("Perform")) {
                histogram.clear();
                histogram.set(
                    image.pixels.data(), 
                    image.pixels.size(), 
                    static_cast<std::size_t>(image.channels_num)
                );
                otsu_binarization_alg.prepare(histogram);
                otsu_binarization_alg.submit(alg_descriptor_ubo_id);
                otsu_binarization_alg.shader.bind();
                alg_perform_fn();
                basic_shader.bind();
            }
        ImGui::End();

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
                    stretching_alg.prepare(image);
                    submit_current_alg_data_fn = &submit_stretching_data_fn;
                    stretching_shader.bind();
                } else {
                    stretching_alg.submit(alg_descriptor_ubo_id);
                    stretching_alg.shader.bind();
                    alg_perform_fn();
                    basic_shader.bind();                
                    submit_current_alg_data_fn = nullptr;
                }
            }
        ImGui::End();

        ImGui::Begin("Equalization");
            ImGui::SliderInt("range", &equalization_alg.descriptor.range, 1, 256);
            if (ImGui::Button("Perform single##2")) {
                histogram.clear();
                histogram.set(
                    image.pixels.data(), 
                    image.pixels.size(), 
                    static_cast<std::size_t>(image.channels_num)
                );
                equalization_alg.prepare(histogram);
                equalization_alg.submit(alg_descriptor_ubo_id);

                equalization_alg.shader.bind();

                alg_perform_fn();

                basic_shader.bind();
            }
            if (static bool state = false; ImGui::Checkbox("Perform continuously##2", &state)) {
                if (state) {
                    histogram.clear();
                    histogram.set(
                        image.pixels.data(), 
                        image.pixels.size(), 
                        static_cast<std::size_t>(image.channels_num)
                    );
                    equalization_alg.prepare(histogram);                
                    equalization_alg.submit(alg_descriptor_ubo_id);

                    equalization_alg.shader.bind();

                    submit_current_alg_data_fn = &submit_equalization_data_fn;
                } else {
                    equalization_alg.shader.bind();
                    alg_perform_fn();
                    basic_shader.bind();                
                    submit_current_alg_data_fn = nullptr;
                }
            }
        ImGui::End();

        ImGui::Begin("Image data details");
            if (ImPlot::BeginPlot(
                    "Histogram",
                    "Possible pixel values",
                    "Count of pixel values in image",
                    ImVec2{-1.F, -1.F}
                )) {
                ImPlot::SetNextFillStyle({.8F, .8F, .8F, 1.F}); // gray
                ImPlot::PlotBars(
                    "From rgb mean", 
                    histogram.mean_sums.data(), 
                    static_cast<int>(histogram.mean_sums.size())
                );
                ImPlot::SetNextFillStyle({.8F, .0F, .0F, 1.F}); // red
                ImPlot::PlotBars(
                    "From red channel", 
                    histogram.r_sums.data(), 
                    static_cast<int>(histogram.r_sums.size())
                );
                ImPlot::SetNextFillStyle({.0F, .8F, .0F, 1.F}); // green
                ImPlot::PlotBars(
                    "From green channel", 
                    histogram.g_sums.data(), 
                    static_cast<int>(histogram.g_sums.size())
                );
                ImPlot::SetNextFillStyle({.0F, .0F, .8F, 1.F}); // blue
                ImPlot::PlotBars(
                    "From blue channel", 
                    histogram.b_sums.data(), 
                    static_cast<int>(histogram.b_sums.size())
                );

                ImPlot::EndPlot();
            }
            if (ImGui::Button("Update", ImVec2{ -1.F, 0.F })) {
                histogram.clear();
                histogram.set(
                    image.pixels.data(), 
                    image.pixels.size(), 
                    static_cast<std::size_t>(image.channels_num)
                );
            }
        ImGui::End();

        ImGui::Begin("Assets");
            static bool assets_window_focused_state_machine { false };
            if (ImGui::IsWindowFocused()) {
                if (!assets_window_focused_state_machine) {
                    asset_imgs_paths = findFilesWithExtensionsInDir(
                        ASSETS_DIR_RELATIVE_PATH, 
                        std::span(ACCEPTED_ASSETS_EXTENSIONS)
                    );   
                    assets_window_focused_state_machine = true;
                }
            } else {
                assets_window_focused_state_machine = false;
            }

            if (ImGui::BeginListBox("Available images")) {
                i32 i{ 0 };
                for (const auto& asset_img_path : asset_imgs_paths.value()) {
                    const auto is_selected = i == selected_asset;
                    if (ImGui::Selectable(asset_img_path.filename().c_str(), is_selected)) {
                        selected_asset = i;
                    }
                    ++i;

                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                
                ImGui::EndListBox();
            }
            if (ImGui::Button("Load selected image")) {
                if (image.update(asset_imgs_paths.value().at(static_cast<std::size_t>(selected_asset)))) {
                    img_texture.unbind(SHCONFIG_2D_TEX_BINDING);
                    img_texture.resize(image.width, image.height);
                    img_texture.update(static_cast<const void*>(image.pixels.data()));
                    img_texture.bind(SHCONFIG_2D_TEX_BINDING);

                    fbo.resize(image.width, image.height);
                }
            }
            if (ImGui::Button("Save to selected image")) {
                image.save(asset_imgs_paths.value().at(static_cast<std::size_t>(selected_asset)));
            }
        ImGui::End();
        
        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.swapBuffers();
    }
    // opengl stuff
    std::array<u32, 3> buffers {{quad_vbo_id, quad_ubo_id, alg_descriptor_ubo_id}};
    glDeleteBuffers(buffers.size(), buffers.data());
    glDeleteVertexArrays(1, &quad_vao_id);
    std::array<u32, 2> textures {{img_texture.tex_id_, fbo.tex_id_}};
    glDeleteTextures(textures.size(), textures.data());
    glDeleteFramebuffers(1, &fbo.fbo_id_);
    basic_shader.deinit();
    threshold_binarization_shader.deinit();
    otsu_binarization_shader.deinit();
    stretching_shader.deinit();
    equalization_shader.deinit();
    // imgui stuff
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    // window
    window.deinit();
}
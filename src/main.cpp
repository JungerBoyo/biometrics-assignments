#include <array>
#include <span>
#include <optional>

#include <Shader.hpp>
#include <Window.hpp>
#include <config.hpp>
#include <Quad.hpp>
#include <Histogram.hpp>
#include <Image.hpp>

#include <glad/glad.h>

#include <stb_image.h>
#include <spdlog/spdlog.h>

#include <imgui.h>
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

template<typename T>
using lim = std::numeric_limits<T>;

namespace fs = std::filesystem;

constexpr i64 MAX_ALG_DESCRIPTOR_SIZE{256};

constexpr std::string_view DEFAULT_ASSET_IMAGE_PATH {"assets/textures/img.jpg"};
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
    u32 quad_tex_id{ 0U };
    u32 alg_descriptor_ubo_id{ 0U };
    u32 texture_fbo_id { 0U };
    u32 aux_quad_tex_id { 0U };

    Shader basic_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/basic_shader/vert.spv",
            "shaders/bin/basic_shader/frag.spv",
        }
    );

    Shader binarization_shader(
        Shader::Type::VERTEX_FRAGMENT,
        {
            "shaders/bin/binarization_shader/vert.spv",
            "shaders/bin/binarization_shader/frag.spv",
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

    glCreateTextures(GL_TEXTURE_2D, 1, &quad_tex_id);
    Image image(DEFAULT_ASSET_IMAGE_PATH);

    // const auto num_levels = static_cast<int>(std::ceil(std::log2(std::min(
    //     static_cast<float>(width), 
    //     static_cast<float>(height)
    // ))));
    glTextureStorage2D(quad_tex_id, 1, GL_RGBA8, image.width, image.height);
    glTextureParameteri(quad_tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(quad_tex_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTextureParameteri(quad_tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    // glTextureParameteri(quad_tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureSubImage2D(
        quad_tex_id,
        0, 0, 0, image.width, image.height,
        GL_RGBA, GL_UNSIGNED_BYTE,
        static_cast<const void*>(image.pixels.data())
    );
    // glGenerateTextureMipmap(quad_tex_id);
    glBindTextureUnit(SHCONFIG_2D_TEX_BINDING, quad_tex_id);

    // Create texture FBO
    glCreateFramebuffers(1, &texture_fbo_id);
    glCreateTextures(GL_TEXTURE_2D, 1, &aux_quad_tex_id);
    glTextureStorage2D(aux_quad_tex_id, 1, GL_RGBA8, image.width, image.height);
    glBindFramebuffer(GL_FRAMEBUFFER, texture_fbo_id);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, aux_quad_tex_id, 0
    );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    // App algorithms data
    //  Binarization
    struct BinarizationDescriptor {
        enum : i32 {
            BINARIZE_CHANNEL_ALL,
            BINARIZE_CHANNEL_R,
            BINARIZE_CHANNEL_G,
            BINARIZE_CHANNEL_B
        };
        float threshold = .0F;
        i32 channel = BINARIZE_CHANNEL_ALL; 
    };
    BinarizationDescriptor binarization_descriptor = {};

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
        glNamedBufferSubData(
            alg_descriptor_ubo_id, 
            0, sizeof(BinarizationDescriptor), 
            static_cast<const void*>(&binarization_descriptor)
        );

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

        ImGui::Begin("Binarization");
            ImGui::SliderFloat(
                "threshold", 
                &binarization_descriptor.threshold,
                0.F,
                1.F
            );
            {
                static u8 states = 0x01;                
                if (ImGui::RadioButton(
                        "binarize all channels", 
                        (states & (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_ALL)) > 0
                    )) {
                    binarization_descriptor.channel = BinarizationDescriptor::BINARIZE_CHANNEL_ALL;
                    states = (states & 0x00) | (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_ALL);
                }
                if (ImGui::RadioButton(
                        "binarize red channel", 
                        (states & (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_R)) > 0
                    )) {
                    binarization_descriptor.channel = BinarizationDescriptor::BINARIZE_CHANNEL_R;
                    states = (states & 0x00) | (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_R);
                }
                if (ImGui::RadioButton(
                        "binarize green channel", 
                        (states & (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_G)) > 0
                    )) {
                    binarization_descriptor.channel = BinarizationDescriptor::BINARIZE_CHANNEL_G;
                    states = (states & 0x00) | (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_G);
                }
                if (ImGui::RadioButton(
                        "binarize blue channel", 
                        (states & (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_B)) > 0
                    )) {
                    binarization_descriptor.channel = BinarizationDescriptor::BINARIZE_CHANNEL_B;
                    states = (states & 0x00) | (0x01 << BinarizationDescriptor::BINARIZE_CHANNEL_B);
                }
            }
            if (ImGui::Button("Perform single")) {
                glBindFramebuffer(GL_FRAMEBUFFER, texture_fbo_id);
                glViewport(0, 0, image.width, image.height);
                binarization_shader.bind();
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
                glCopyTextureSubImage2D(quad_tex_id, 0, 0, 0, 0, 0, image.width, image.height);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                basic_shader.bind();
            }
            if (static bool state = false; ImGui::Checkbox("Perform continuously", &state)) {
                if (state) {
                    binarization_shader.bind();
                } else {
                    glBindFramebuffer(GL_FRAMEBUFFER, texture_fbo_id);
                    glViewport(0, 0, image.width, image.height);
                    binarization_shader.bind();
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
                    glCopyTextureSubImage2D(quad_tex_id, 0, 0, 0, 0, 0, image.width, image.height);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    basic_shader.bind();
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
                    glDeleteTextures(1, &quad_tex_id);
                    glCreateTextures(GL_TEXTURE_2D, 1, &quad_tex_id);
                    
                    glTextureStorage2D(quad_tex_id, 1, GL_RGBA8, image.width, image.height);
                    glTextureParameteri(quad_tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTextureParameteri(quad_tex_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTextureSubImage2D(
                        quad_tex_id,
                        0, 0, 0, image.width, image.height,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        static_cast<const void*>(image.pixels.data())
                    );
                    glBindTextureUnit(SHCONFIG_2D_TEX_BINDING, quad_tex_id);
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
    std::array<u32, 2> textures {{quad_tex_id, aux_quad_tex_id}};
    glDeleteTextures(textures.size(), textures.data());
    glDeleteFramebuffers(1, &texture_fbo_id);
    basic_shader.deinit();
    binarization_shader.deinit();
    // imgui stuff
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    // window
    window.deinit();
}
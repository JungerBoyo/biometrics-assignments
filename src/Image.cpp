#include "Image.hpp"

#include <exception>

#include <stb_image.h>
#include <stb_image_write.h>

#include <spdlog/spdlog.h>

using namespace bm;

constexpr int DESIRED_CHANNELS_NUM{ 4 };

Image::Image(const std::filesystem::path& image_path) {
    if (update(image_path)) {
        current_path = image_path;
    }
}

void Image::save(const std::filesystem::path& save_to_path) const {
    if (!std::filesystem::is_regular_file(save_to_path)) {
        spdlog::error("File can only be saved to regular file path", save_to_path.c_str());
        return;
    }

    auto extension = save_to_path.extension().string();
    std::transform(
        extension.begin(), 
        extension.end(), 
        extension.begin(), 
        [](u8 c) { return std::tolower(c); }
    );

    if (std::strcmp(extension.c_str(), ".jpg") == 0 || std::strcmp(extension.c_str(), ".jpeg") == 0) {
        if (stbi_write_jpg(
            save_to_path.c_str(), 
            width, height, channels_num, 
            static_cast<const void*>(pixels.data()),
            100
        ) == 0) {
            spdlog::error("Failed to write image as jpg to {}", save_to_path.c_str());
            return;
        }
    } else if (std::strcmp(extension.c_str(), ".png") == 0) {
        if (stbi_write_png(
            save_to_path.c_str(),
            width, height, channels_num, 
            static_cast<const void*>(pixels.data()),
            width * channels_num
        ) == 0) {
            spdlog::error("Failed to write image as png to {}", save_to_path.c_str());
            return;
        }
    } else {
        spdlog::error("Wrong file extension can save only jpg/jpeg and png (passed {})", extension);
        return;
    }

}

bool Image::update(const std::filesystem::path& update_from_path) {
    if (!std::filesystem::is_regular_file(update_from_path)) {
        spdlog::error("File can only be updated from regular file path (passed {})", update_from_path.c_str());
        return false;
    }

    const auto str_img_path = update_from_path.string();

    int width       { 0 };
    int height      { 0 };
    int channels_num{ 0 };
    auto* img_ptr = stbi_load(
        str_img_path.c_str(), 
        &width, 
        &height, 
        &channels_num, 
        DESIRED_CHANNELS_NUM
    );

    if (img_ptr == nullptr) {
        spdlog::error("Failed to load image from {}", update_from_path.c_str());
        return false;
    }

    this->width = width;
    this->height = height;
    this->channels_num = DESIRED_CHANNELS_NUM;
    pixels.resize(static_cast<std::size_t>(width * height * DESIRED_CHANNELS_NUM));

    std::copy(
        img_ptr, std::next(img_ptr, static_cast<std::ptrdiff_t>(pixels.size())), 
        pixels.begin()
    );

    stbi_image_free(img_ptr);
    img_ptr = nullptr;

    return true;
}
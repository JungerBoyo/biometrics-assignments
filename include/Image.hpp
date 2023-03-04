#ifndef BM_IMAGE_HPP
#define BM_IMAGE_HPP

#include <vector>
#include <filesystem>
#include "Types.hpp"

namespace bm {

struct Image {
    i32 width{ 0 };
    i32 height{ 0 };
    i32 channels_num{ 0 };
    std::vector<u8> pixels;

    std::filesystem::path current_path; 

    Image(const std::filesystem::path& image_path);

    void save(const std::filesystem::path& save_to_path);
    bool update(const std::filesystem::path& update_from_path);
};

}

#endif
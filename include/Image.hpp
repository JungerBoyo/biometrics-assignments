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

    void save(const std::filesystem::path& save_to_path) const;
    bool update(const std::filesystem::path& update_from_path);

    template<typename T>
    requires std::same_as<T, f32> || std::same_as<T, u8>
    auto minMax() const {
        Pixel<u8> min{ lim<u8>::max(), lim<u8>::max(), lim<u8>::max() };
        Pixel<u8> max{ 0, 0, 0 };
        for (auto i = 0UL; i < pixels.size(); i+=static_cast<std::size_t>(channels_num)) {
            const auto r = pixels[i + 0];
            if (min.r > r) { min.r = r; }
            if (max.r < r) { max.r = r; }
            
            const auto g = pixels[i + 1];
            if (min.g > g) { min.g = g; }
            if (max.g < g) { max.g = g; }
           
            const auto b = pixels[i + 2];
            if (min.b > b) { min.b = b; }
            if (max.b < b) { max.b = b; }
        }

        if constexpr (std::same_as<T, f32>) {
            return std::make_tuple(
                Pixel<T>{ 
                    static_cast<T>(min.r) / 255.F,
                    static_cast<T>(min.g) / 255.F,
                    static_cast<T>(min.b) / 255.F,
                },
                Pixel<T>{ 
                    static_cast<T>(max.r) / 255.F,
                    static_cast<T>(max.g) / 255.F,
                    static_cast<T>(max.b) / 255.F,
                } 
            );
        } else {
            return std::make_tuple(min, max);
        }

    }


};

}

#endif
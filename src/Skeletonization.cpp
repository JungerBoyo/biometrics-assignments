#include <Skeletonization.hpp>
#include <array>
#include <vector>
#include <fstream>

using namespace bm;

enum OffsetIndices : std::size_t {
    // corner
    NW, NE, SW, SE,
    // edge 
    N, W, E, S,
};

static void getPxFromOffsets(const std::array<i32, 8>& offsets, const std::vector<u8>& bmp, std::array<u8, 8>& values, i32 i) {
    // corner
    values[NW] = (i + offsets[0]) > 0 && (i + offsets[0]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[0])] > 0 : 0;
    values[NE] = (i + offsets[2]) > 0 && (i + offsets[2]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[2])] > 0 : 0;
    values[SW] = (i + offsets[5]) > 0 && (i + offsets[5]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[5])] > 0 : 0;
    values[SE] = (i + offsets[7]) > 0 && (i + offsets[7]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[7])] > 0 : 0;
    // edge 
    values[N]  = (i + offsets[1]) > 0 && (i + offsets[1]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[1])] > 0 : 0;
    values[W]  = (i + offsets[3]) > 0 && (i + offsets[3]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[3])] > 0 : 0;
    values[E]  = (i + offsets[4]) > 0 && (i + offsets[4]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[4])] > 0 : 0;
    values[S]  = (i + offsets[6]) > 0 && (i + offsets[6]) < bmp.size() ? bmp[static_cast<std::size_t>(i + offsets[6])] > 0 : 0;
}

void bm::performKMMSkeletonization(void* pixels, i32 width, i32 height, i32 channels_num) {
     
	auto *ptr = static_cast<u8*>(pixels);

    const auto stride = width * channels_num;
    const auto pixels_size = stride * height;

    const std::array<i32, 8> offsets{{
        -width - 1, -width, -width + 1,
                 -1, /*    0,*/1,
        width - 1, width, width + 1
    }};

    std::vector<u8> tmp_bitmap(static_cast<std::size_t>(width * height));
    for (i32 i{0}, j{0}; i < pixels_size; i += channels_num, ++j) {
        tmp_bitmap[j] = ptr[i] == 0 ? 1U : 0U;
    }

    for (bool done{ false }; !done;) {
        done = true;

        for (i32 j{0}; j<tmp_bitmap.size(); ++j) {
            if (tmp_bitmap[j] == 0) { continue; }

            std::array<u8, 8> values;
            getPxFromOffsets(offsets, tmp_bitmap, values, j);

            const bool sticks_to_bg_with_edge = (values[N] & values[W] & values[E] & values[S]) == 0;
            const bool sticks_to_bg_with_corner = (values[NW] & values[SW] & values[NE] & values[SE]) == 0;
            if (sticks_to_bg_with_edge) {
                tmp_bitmap[j] = 2;
            } else if (sticks_to_bg_with_corner) {
                tmp_bitmap[j] = 3;
            }
        }

        constexpr std::array<u16, 16> contours_deletion_arr{{
            0b0001000100001011U, 
            0b0000000000001010U,
            0b0000000000000000U,
            0b1000000010001000U,
            0b0000000000000000U,
            0b0000000000000000U,
            0b0000000000000000U,
            0b1000000010000000U,
            0b0001000100000000U,
            0b0000000000000000U,
            0b0000000000000000U,
            0b0000000000000000U,
            0b1101000000000000U,
            0b0000000000000000U,
            0b1100000000000000U,
            0b1000000000000000U
        }};

        for (i32 j{0}; j<tmp_bitmap.size(); ++j) {
            if (tmp_bitmap[j] == 0) { continue; }

            std::array<u8, 8> values;
            getPxFromOffsets(offsets, tmp_bitmap, values, j);

            const u8 sum = 
                values[NW] * 128 + values[N] * 1 + values[NE] * 2 +
                values[W]  * 64                  + values[E]  * 4 +
                values[SW] * 32 + values[S] * 16 + values[SE] * 8;

            if ((contours_deletion_arr[sum / 16] & (0x8000U >> (sum % 16))) > 0U) {
                tmp_bitmap[j] = 4;
                done = false;
            }        
        }
        for (auto& value : tmp_bitmap) {
            if (value == 4) { value = 0; }
        }

        constexpr std::array<u16, 16> full_deletion_arr{{
            0b0001010100001111U,   
            0b0000111100001111U, 
            0b0000000000000000U, 
            0b1000111110001111U, 
            0b0101010100000101U, 
            0b1101111111011111U, 
            0b0101010100000101U, 
            0b1101111111011111U, 
            0b0001010100000101U, 
            0b0000010100000101U, 
            0b0000000000000000U, 
            0b0000010100000101U, 
            0b1101010100000101U, 
            0b1101111111011111U, 
            0b1101010100000101U, 
            0b1101111111011111U
        }};

        constexpr std::array<u8, 2> pixel_types{{2, 3}};
        for (const auto pixel_type : pixel_types) {
            for (i32 j{0}; j<tmp_bitmap.size(); ++j) {
                if (tmp_bitmap[j] == 0) { continue; }

                std::array<u8, 8> values;
                getPxFromOffsets(offsets, tmp_bitmap, values, j);

                if (tmp_bitmap[j] == pixel_type) {
                    const u8 sum = 
                        values[NW] * 128 + values[N] * 1 + values[NE] * 2 +
                        values[W]  * 64                  + values[E]  * 4 +
                        values[SW] * 32 + values[S] * 16 + values[SE] * 8;

                    if ((full_deletion_arr[sum / 16] & (0x8000U >> (sum % 16))) > 0U) {
                        tmp_bitmap[j] = 0;
                        done = false;
                    } else {
                        tmp_bitmap[j] = 1;
                    }       
                }        
            }
        }
    }

    for (i32 i{0}, j{0}; i < pixels_size; i += channels_num, ++j) {
        ptr[i + 0] = ptr[i + 1] = ptr[i + 2] = tmp_bitmap[j] == 1 ? 0U : 255U;
    }
}

void bm::performK3MSkeletonization(void* pixels, i32 width, i32 height, i32 channels_num) {
    constexpr std::array<u16, 16> P0 {{    
        0b0001001100001011, 
        0b0000000010001011,
        0b0000000000000000,
        0b1000000010001011,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000000000000,
        0b1000000010001011,
        0b0101000100000001,
        0b0000000000000001,
        0b0000000000000000,
        0b0000000000000001,
        0b1101000100000001,
        0b0000000000000001,
        0b1101000100000001,
        0b1101000111011110
    }};
    constexpr std::array<u16, 16> P1 {{    
        0b0000000100000010, 
        0b0000000000001000,
        0b0000000000000000,
        0b0000000010000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000000000000,
        0b0001000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0100000000000000,
        0b0000000000000000,
        0b1000000000000000,
        0b0000000000000000
    }};
    constexpr std::array<u16, 16> P2 {{    
        0b0000000100000011, 
        0b0000000000001010,
        0b0000000000000000,
        0b0000000010001000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000010000000,
        0b0001000100000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0101000000000000,
        0b0000000000000000,
        0b1100000000000000,
        0b1000000000000000
    }};
    constexpr std::array<u16, 16> P3 {{    
        0b0000000100000011, 
        0b0000000000001011,
        0b0000000000000000,
        0b0000000010001010,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000010001000,
        0b0001000100000001,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0101000100000000,
        0b0000000000000000,
        0b1101000000000000,
        0b1100000010000000
    }};
    constexpr std::array<u16, 16> P4 {{    
        0b0000000100000011, 
        0b0000000000001011,
        0b0000000000000000,
        0b0000000010001011,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000010001010,
        0b0001000100000001,
        0b0000000000000001,
        0b0000000000000000,
        0b0000000000000000,
        0b0101000100000001,
        0b0000000000000000,
        0b1101000100000000,
        0b1101000011001000
    }};
    constexpr std::array<u16, 16> P5 {{    
        0b0000000100000011, 
        0b0000000000001011,
        0b0000000000000000,
        0b0000000010001011,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b1000000010001010,
        0b0001000100000001,
        0b0000000000000001,
        0b0000000000000000,
        0b0000000000000001,
        0b0101000100000001,
        0b0000000000000000,
        0b1101000100000001,
        0b1101000011011010
    }};

    std::array<const std::array<u16, 16>*, 5> main_phases{{&P1, &P2, &P3, &P4, &P5}};

	auto *ptr = static_cast<u8*>(pixels);

    const auto stride = width * channels_num;
    const auto pixels_size = stride * height;

    const std::array<i32, 8> offsets{{
        -stride - channels_num, -stride, -stride+ channels_num,
                 -channels_num, /*    0,*/channels_num,
        stride - channels_num, stride, stride + channels_num
    }};

    std::vector<i32> border_pixels_indices;
    for (bool done{ false }; !done;) {
        done = true;

        for (i32 i{0}; i < pixels_size; i += channels_num) {
            if (ptr[i] == 255U) { continue; }

            std::array<u8, 8> values;
            // corner
            values[NW] = (i + offsets[0]) > 0 && (i + offsets[0]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[0])] == 0 : 0;
            values[NE] = (i + offsets[2]) > 0 && (i + offsets[2]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[2])] == 0 : 0;
            values[SW] = (i + offsets[5]) > 0 && (i + offsets[5]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[5])] == 0 : 0;
            values[SE] = (i + offsets[7]) > 0 && (i + offsets[7]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[7])] == 0 : 0;
            // edge 
            values[N]  = (i + offsets[1]) > 0 && (i + offsets[1]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[1])] == 0 : 0;
            values[W]  = (i + offsets[3]) > 0 && (i + offsets[3]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[3])] == 0 : 0;
            values[E]  = (i + offsets[4]) > 0 && (i + offsets[4]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[4])] == 0 : 0;
            values[S]  = (i + offsets[6]) > 0 && (i + offsets[6]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[6])] == 0 : 0;

            const u8 sum = 
                values[NW] * 128 + values[N] * 1 + values[NE] * 2 +
                values[W]  * 64                  + values[E]  * 4 +
                values[SW] * 32 + values[S] * 16 + values[SE] * 8;

            if ((P0[sum / 16] & (0x8000U >> (sum % 16))) > 0U) {
                border_pixels_indices.push_back(i);
            }
        }

        for (const auto PX : main_phases) {
            for (const auto i : border_pixels_indices) {
                std::array<u8, 8> values;
                // corner
                values[NW] = (i + offsets[0]) > 0 && (i + offsets[0]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[0])] == 0 : 0;
                values[NE] = (i + offsets[2]) > 0 && (i + offsets[2]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[2])] == 0 : 0;
                values[SW] = (i + offsets[5]) > 0 && (i + offsets[5]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[5])] == 0 : 0;
                values[SE] = (i + offsets[7]) > 0 && (i + offsets[7]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[7])] == 0 : 0;
                // edge 
                values[N]  = (i + offsets[1]) > 0 && (i + offsets[1]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[1])] == 0 : 0;
                values[W]  = (i + offsets[3]) > 0 && (i + offsets[3]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[3])] == 0 : 0;
                values[E]  = (i + offsets[4]) > 0 && (i + offsets[4]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[4])] == 0 : 0;
                values[S]  = (i + offsets[6]) > 0 && (i + offsets[6]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[6])] == 0 : 0;
                
                const u8 sum = 
                    values[NW] * 128 + values[N] * 1 + values[NE] * 2 +
                    values[W]  * 64                  + values[E]  * 4 +
                    values[SW] * 32 + values[S] * 16 + values[SE] * 8;

                if ((PX->at(sum / 16) & (0x8000U >> (sum % 16))) > 0U) {
                    ptr[i + 0] = ptr[i + 1] = ptr[i + 2] = 255U;
                    done = false;
                }
            }
        }

        border_pixels_indices.clear();
    }
    for (i32 i{0}; i < pixels_size; i += channels_num) {
        if (ptr[i] == 255U) { continue; }

        std::array<u8, 8> values;
        // corner
        values[NW] = (i + offsets[0]) > 0 && (i + offsets[0]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[0])] == 0 : 0;
        values[NE] = (i + offsets[2]) > 0 && (i + offsets[2]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[2])] == 0 : 0;
        values[SW] = (i + offsets[5]) > 0 && (i + offsets[5]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[5])] == 0 : 0;
        values[SE] = (i + offsets[7]) > 0 && (i + offsets[7]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[7])] == 0 : 0;
        // edge 
        values[N]  = (i + offsets[1]) > 0 && (i + offsets[1]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[1])] == 0 : 0;
        values[W]  = (i + offsets[3]) > 0 && (i + offsets[3]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[3])] == 0 : 0;
        values[E]  = (i + offsets[4]) > 0 && (i + offsets[4]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[4])] == 0 : 0;
        values[S]  = (i + offsets[6]) > 0 && (i + offsets[6]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[6])] == 0 : 0;

        const u8 sum = 
            values[NW] * 128 + values[N] * 1 + values[NE] * 2 +
            values[W]  * 64                  + values[E]  * 4 +
            values[SW] * 32 + values[S] * 16 + values[SE] * 8;

        if ((P0[sum / 16] & (0x8000U >> (sum % 16))) > 0U) {
            ptr[i + 0] = ptr[i + 1] = ptr[i + 2] = 255U;
        }
    }
}

void bm::performCrossingNumber(void* pixels, i32 width, i32 height, i32 channels_num, const fs::path& output_file_path) {
	auto *ptr = static_cast<u8*>(pixels);

    const auto stride = width * channels_num;
    const auto pixels_size = stride * height;

    const std::array<i32, 8> offsets{{
        -stride - channels_num, -stride, -stride+ channels_num,
                 -channels_num, /*    0,*/channels_num,
        stride - channels_num, stride, stride + channels_num
    }};

    enum MinutiaeType : u8{
        POINT = 0,
        RIDGE_ENDING_POINT = 1,
        CONTINUING_RIDGE_POINT = 2,
        BIFURCATION_POINT = 3,
        CROSSING_POINT = 4
    };

    std::array<u32, 5> minutiae_histogram{{0U, 0U, 0U, 0U, 0U}};
    std::array<u8[3], 5> minutiae_colors{{
        {255U, 0U, 0U}, 
        {0U, 255U, 0U}, 
        {0U, 0U, 255U}, 
        {255U, 0U, 255U}, 
        {0U, 255U, 255U}
    }};

    for (i32 i{0}; i < pixels_size; i += channels_num) {
        if (ptr[i] == 255U) { continue; }

        std::array<u8, 8> values;
        // corner
        values[0] = (i + offsets[0]) > 0 && (i + offsets[0]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[0])] == 0 : 0;
        values[2] = (i + offsets[2]) > 0 && (i + offsets[2]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[2])] == 0 : 0;
        values[5] = (i + offsets[5]) > 0 && (i + offsets[5]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[5])] == 0 : 0;
        values[7] = (i + offsets[7]) > 0 && (i + offsets[7]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[7])] == 0 : 0;
        // edge 
        values[1]  = (i + offsets[1]) > 0 && (i + offsets[1]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[1])] == 0 : 0;
        values[3]  = (i + offsets[3]) > 0 && (i + offsets[3]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[3])] == 0 : 0;
        values[4]  = (i + offsets[4]) > 0 && (i + offsets[4]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[4])] == 0 : 0;
        values[6]  = (i + offsets[6]) > 0 && (i + offsets[6]) < pixels_size ? ptr[static_cast<std::size_t>(i + offsets[6])] == 0 : 0;
        
        const u8 sum = std::accumulate(values.cbegin(), values.cend(), 0);

        if (sum < 5) {
            ++minutiae_histogram[sum];
            const u8* minutiae_color = minutiae_colors[sum];

            ptr[i + 0] = minutiae_color[0];
            ptr[i + 1] = minutiae_color[1];
            ptr[i + 2] = minutiae_color[2];

            i32 j{0};
            for (const auto value : values) {
                if (value == 1) {
                    ptr[i + offsets[j] + 0] = minutiae_color[0];
                    ptr[i + offsets[j] + 1] = minutiae_color[1];
                    ptr[i + offsets[j] + 2] = minutiae_color[2];
                }
                ++j;                
            }
        }

    }

    std::ofstream stream(output_file_path);

    if (stream.is_open()) {
        stream << "points = " << minutiae_histogram[POINT] << '\n';
        stream << "ridge_ending_points = " << minutiae_histogram[RIDGE_ENDING_POINT] << '\n';
        stream << "continuing_ridge_points = " << minutiae_histogram[CONTINUING_RIDGE_POINT] << '\n';
        stream << "bifurcation_points = " << minutiae_histogram[BIFURCATION_POINT] << '\n';
        stream << "crossing_points = " << minutiae_histogram[CROSSING_POINT];
        
        stream.close();
    }
}
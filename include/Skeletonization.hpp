#ifndef BM_SKELETONIZATION_HPP
#define BM_SKELETONIZATION_HPP

#include "Image.hpp"

namespace bm {

void performKMMSkeletonization(void* pixels, i32 width, i32 height, i32 channels_num);
void performK3MSkeletonization(void* pixels, i32 width, i32 height, i32 channels_num);
void performCrossingNumber(void* pixels, i32 width, i32 height, i32 channels_num, const fs::path& output_file_path);

}

#endif
#ifndef BM_TEXTURE2D_HPP
#define BM_TEXTURE2D_HPP

#include "Types.hpp"

namespace bm {

struct Texture2D {
    struct Config {
        i32 width;
        i32 height;
        u32 internal_fmt;
        u32 fmt;
        u32 type;
        u32 wrap_s;
        u32 wrap_t;
        u32 min_filter;
        u32 mag_filter;
        bool mipmap;
    };
    u32 tex_id_{ 0 };
    Config config;

    Texture2D(Config config);
    void resize(i32 width, i32 height);
    void update(const void* data) const;
    void deinit();

    void bind(u32 tex_unit) const;
    void unbind(u32 tex_unit) const;
};

}

#endif
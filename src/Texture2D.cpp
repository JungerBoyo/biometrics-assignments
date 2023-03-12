#include "Texture2D.hpp"

#include <cmath>

#include <glad/glad.h>

using namespace bm;

static void configure(u32 tex_id, Texture2D::Config config) {
    const auto num_levels = config.mipmap ? static_cast<i32>(std::ceil(std::log2(std::min(
        static_cast<f32>(config.width), 
        static_cast<f32>(config.height)
    )))) : 1;
    
    glTextureStorage2D(tex_id, num_levels, config.internal_fmt, config.width, config.height);
    if (config.wrap_s != 0) {
        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, config.wrap_s);
    }
    if (config.wrap_t != 0) {
        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, config.wrap_t);
    }
    if (config.min_filter != 0) {
        glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, config.min_filter);
    }
    if (config.mag_filter != 0) {
        glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, config.mag_filter);
    }
    if (config.mipmap) {
        glGenerateTextureMipmap(tex_id);
    }
}

Texture2D::Texture2D(Config config) : config(config) {
    glCreateTextures(GL_TEXTURE_2D, 1, &tex_id_);
    configure(tex_id_, config);
}
void Texture2D::resize(i32 width, i32 height) {
    deinit();
    glCreateTextures(GL_TEXTURE_2D, 1, &tex_id_);
    config.width = width;
    config.height = height;
    configure(tex_id_, config);
}
void Texture2D::update(const void* data) const {
    glTextureSubImage2D(
        tex_id_,
        0, 0, 0, config.width, config.height,
        config.fmt, config.type,
        data 
    );
}
void Texture2D::deinit() {
    glDeleteTextures(1, &tex_id_);
}
void Texture2D::bind(u32 tex_unit) const {
    glBindTextureUnit(tex_unit, tex_id_);
}
void Texture2D::unbind(u32 tex_unit) const {
    glBindTextureUnit(tex_unit, 0);
}
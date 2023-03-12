#include "Framebuffer.hpp"

#include <glad/glad.h>

using namespace bm;

FBO::FBO(Config config) : config(config) {
    glCreateFramebuffers(1, &fbo_id_);
    glCreateTextures(GL_TEXTURE_2D, 1, &tex_id_);
    glTextureStorage2D(tex_id_, 1, config.internal_fmt, config.width, config.height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, 
        config.color_attachment,
        GL_TEXTURE_2D, tex_id_, 0
    );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::resize(i32 width, i32 height) {
    if (config.width == width && config.height == height) {
        return;
    }

    glDeleteTextures(1, &tex_id_);
    glCreateTextures(GL_TEXTURE_2D, 1, &tex_id_);
    glTextureStorage2D(tex_id_, 1, config.internal_fmt, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, 
        config.color_attachment,
        GL_TEXTURE_2D, tex_id_, 0
    );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    config.width = width;
    config.height = height;
}

void FBO::deinit() {
    glDeleteTextures(1, &tex_id_);
    glDeleteFramebuffers(1, &fbo_id_);
    tex_id_ = 0;
    fbo_id_ = 0;
}

void FBO::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
}
void FBO::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
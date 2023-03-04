#include "Quad.hpp"

#include <glad/glad.h>

using namespace bm;

void bm::initQuad(u32 quad_vbo_id, u32 quad_vao_id) {
    glNamedBufferStorage(
        quad_vbo_id, 
        sizeof(QUAD_VERTICES), 
        static_cast<const void*>(QUAD_VERTICES.data()), 
        0
    );

    constexpr u32 attribs_binding{ 0 };
    constexpr u32 base_size{ 4 }; // float/unsigned int
    constexpr u32 num_components{ 2 };
    
    u32 relative_offset{ 0 };

    glEnableVertexArrayAttrib(quad_vao_id, SHCONFIG_IN_POSITION_LOCATION);
    glVertexArrayAttribFormat(
        quad_vao_id, 
        SHCONFIG_IN_POSITION_LOCATION,
        num_components,
        GL_FLOAT,
        GL_FALSE,
        relative_offset
    );
    glVertexArrayAttribBinding(quad_vao_id, SHCONFIG_IN_POSITION_LOCATION, attribs_binding);
    relative_offset += num_components * base_size;
    
    glEnableVertexArrayAttrib(quad_vao_id, SHCONFIG_IN_TEXCOORD_LOCATION);
    glVertexArrayAttribFormat(
        quad_vao_id, 
        SHCONFIG_IN_TEXCOORD_LOCATION,
        num_components,
        GL_FLOAT,
        GL_FALSE,
        relative_offset
    );
    glVertexArrayAttribBinding(quad_vao_id, SHCONFIG_IN_TEXCOORD_LOCATION, attribs_binding);
    relative_offset += num_components * base_size;

    glVertexArrayBindingDivisor(quad_vao_id, attribs_binding, 0);
    glVertexArrayVertexBuffer(
        quad_vao_id, 
        attribs_binding, 
        quad_vbo_id, 0,
        static_cast<int>(relative_offset)
    );
}
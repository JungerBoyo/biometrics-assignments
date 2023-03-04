#ifndef BM_QUAD_HPP
#define BM_QUAD_HPP

#include <array>
#include <tuple>
#include "Types.hpp"

namespace bm {

struct QuadVertex {
    f32 position_x;
    f32 position_y;
    f32 texcoord_x;
    f32 texcoord_y;
};

static constexpr std::array<QuadVertex, 6UL> QUAD_VERTICES{{
    QuadVertex{-1.F,-1.F, 0.F, 1.F},
    QuadVertex{ 1.F,-1.F, 1.F, 1.F},
    QuadVertex{ 1.F, 1.F, 1.F, 0.F},
    QuadVertex{-1.F,-1.F, 0.F, 1.F},
    QuadVertex{ 1.F, 1.F, 1.F, 0.F},
    QuadVertex{-1.F, 1.F, 0.F, 0.F},
}};

void initQuad(u32 quad_vbo_id, u32 quad_vao_id);

}

#endif
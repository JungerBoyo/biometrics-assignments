#ifndef BM_FRAMEBUFFER_HPP
#define BM_FRAMEBUFFER_HPP

#include "Types.hpp"

namespace bm {

struct FBO {
	struct Config {
		i32 width;
		i32 height;
		u32 color_attachment;
		u32 internal_fmt;
	};
	u32 fbo_id_;
	u32 tex_id_;
	Config config;

	FBO(Config config);
	void resize(i32 width, i32 height);
	void deinit();

	void bind() const;
	void unbind() const;
};

} // namespace bm

#endif
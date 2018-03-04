#pragma once

namespace amp_shader{

class amp_shader : public amp_shader_base<amp_shader>{

	float_2 resolution;

public:

	amp_shader(const inputs& in):resolution(in.resolution){}

	unorm_4 operator()(const float_2& coord)const restrict(amp){
		return unorm4(unorm3(coord.x / resolution.x), 1.f);
	}

};

namespace config{

static constexpr float time_per_frame = 0.f;
static constexpr int width = 512;
static constexpr int height = width;
static constexpr bool prerender = true;
using shader = amp_shader;

}

}
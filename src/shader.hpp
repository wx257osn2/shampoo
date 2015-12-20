#pragma once

namespace amp_shader{

class amp_shader : public amp_shader_base<amp_shader>{

	float_2 resolution;

public:

	amp_shader(const inputs& in):resolution(in.resolution){}

	unorm_4 operator()(const float_2& coord)const restrict(amp){
		return vec4<unorm>(vec3<unorm>(coord.x / resolution.x), 1.f);
	}

};

namespace config{

static constexpr float time_per_frame = 0.f;
static constexpr unsigned int width = 512u;
static constexpr unsigned int height = width;
static constexpr bool prerender = true;
using shader = amp_shader;

}

}
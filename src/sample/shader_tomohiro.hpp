#pragma once
//http://www.demoscene.jp/?p=811

namespace amp_shader{

class sample_ray_marching : public amp_shader_base<sample_ray_marching>{

struct camera{
	float_3 get_side()const restrict(cpu, amp){return cross(dir, up);}
	float_3 pos;
	float_3 dir;
	float_3 up;
	__declspec(property(get = get_side)) float_3 side;
};

float_2 resolution;

const camera cam = {{0.f, 0.f, 3.f}, {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f}};
const float focus = 1.8f;

float length_n(const float_3& v, float n)const restrict(amp){
	const float_3 tmp = pow(abs(v), vec3(n));
	return pow(tmp.x + tmp.y + tmp.z, 1.f / n);
}

float_3 clamp(const float_3& pos)const restrict(amp){
	return mod(pos, 8.f) - 4.f;
}

float distance_function(const float_3& pos)const restrict(amp){
	return length_n(clamp(pos), 4.f) - 1.f;
}

float_3 get_normal(float_3 p)const restrict(amp){
	const float d = .0001f;
	return
		normalize(
			vec3(
				distance_function(p+vec3(d,0.f,0.f))-distance_function(p+vec3(-d,0.f,0.f)),
				distance_function(p+vec3(0.f,d,0.f))-distance_function(p+vec3(0.f,-d,0.f)),
				distance_function(p+vec3(0.f,0.f,d))-distance_function(p+vec3(0.f,0.f,-d))
			)
		);
}

public:

sample_ray_marching(const inputs& in):resolution(in.resolution){}

unorm_4 operator()(const float_2& coord)const restrict(amp){
	const float_2 pos = (coord.xy * 2.f - resolution) / min(resolution.x, resolution.y);
	const auto raydir = normalize(cam.side * pos.x + cam.up * pos.y + cam.dir * focus);
	float t = 0.f, d;
	auto pos_on_ray = cam.pos;
	for(int i = 0; i < 64; ++i){
		d = distance_function(pos_on_ray);
		t += d;
		pos_on_ray = cam.pos + t * raydir;
	}
	if(abs(d) < .001f)
		return unorm4(get_normal(pos_on_ray), 1.f);
	else
		return unorm4(0.f);
}

};

namespace config{

static constexpr float time_per_frame = 0.f;
static constexpr int width = 512;
static constexpr int height = width;
static constexpr bool prerender = true;
using shader = sample_ray_marching;

}

}
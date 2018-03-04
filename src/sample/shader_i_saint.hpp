#pragma once
//http://i-saint.hatenablog.com/entry/2013/08/20/003046

namespace amp_shader{

class i_saint_ray_marching : public amp_shader_base<i_saint_ray_marching>{

struct camera{
	float_3 get_side()const restrict(cpu, amp){return cross(dir, up);}
	float_3 pos;
	float_3 dir;
	float_3 up;
	__declspec(property(get = get_side)) float_3 side;
};

float_2 resolution;
float time;
const float focus = 1.8f;

float box( const float_3& p, const float_3& b )const restrict(amp){
	auto d = abs(p) - b;
	return min(max(d.x,max(d.y,d.z)),0.f) + length(max(d,0.f));
}

float box( const float_2& p, const float_2& b )const restrict(amp){
	auto d = abs(p) - b;
	return min(max(d.x,d.y),0.f) + length(max(d,0.f));
}


float_3 nrand3( const float_2& co )const restrict(amp){
	const auto a = fract( cos( co.x*8.3e-3f + co.y )*vec3(1.3e5f, 4.7e5f, 2.9e5f) );
	const auto b = fract( sin( co.x*0.3e-3f + co.y )*vec3(8.1e5f, 1.0e5f, 0.1e5f) );
	return mix(a, b, 0.5);
}

float distance_function(float_3 p)const restrict(amp){
	const auto h = 1.8f;
	const auto rh = .5f;
	const auto grid = .4f;
	const auto grid_half = grid*.5f;
	const auto cube = .175f;
	const auto g1 = vec3(ceil((p.x)/grid), ceil((p.y)/grid), ceil((p.z)/grid));
	const auto rxz =  nrand3(g1.xz);
	const auto ryz =  nrand3(g1.yz);

	p = -abs(p);
	const auto di = ceil(p/4.8f);
	p.y += di.x*1.f;
	p.x += di.y*1.2f;
	p.xy = mod(p.xy, -4.8f);

	const auto gap = vec2(rxz.x*rh, ryz.y*rh);
	const float d1 = p.y + h + gap.x;
	const float d2 = p.x + h + gap.y;

	const auto p1 = mod(p.xz, vec2(grid)) - vec2(grid_half);
	const float c1 = box(p1,vec2(cube));

	const auto p2 = mod(p.yz, vec2(grid)) - vec2(grid_half);
	const float c2 = box(p2,vec2(cube));

	return max(max(c1,d1), max(c2,d2));
}



float_3 gen_normal(float_3 p)const restrict(amp){
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

i_saint_ray_marching(const inputs& in):resolution(in.resolution), time(in.time){}

unorm_4 operator()(const float_2& coord)const restrict(amp){
	const float_2 pos = (coord.xy * 2.f - resolution) / min(resolution.x, resolution.y);
	const camera cam = {vec3(-.5f,0.f,3.f) - vec3(0.f,0.f,time*3.f), normalize(vec3(.3f, 0.f, -1.f)), normalize(vec3(.5f, 1.f, 0.f))};
	const auto raydir = normalize(cam.side * pos.x + cam.up * pos.y + cam.dir * focus);
	auto ray = cam.pos;
	int march = 0;
	float d = 0.f;

	float total_d = 0.f;
	constexpr int max_march = 64;
	constexpr float max_dist = 100.f;
	for(;march < max_march; ++march) {
		d = distance_function(ray);
		total_d += d;
		ray += raydir * d;
		if(d<0.001)
			break;
		if(total_d > max_dist) {
			total_d = max_dist;
			march = max_march - 1;
			break;
		}
	}

	float glow = 0.f;
	{
		const float s = .0075f;
		const auto n1 = gen_normal(ray);
		const auto n2 = gen_normal(ray + vec3(s, 0.f, 0.f));
		const auto n3 = gen_normal(ray + vec3(0.f, s, 0.f));
		glow = (1.f - abs(dot(cam.dir, n1))) * .5f;
		glow += .6f * (dot(n1, n2) < .8f || dot(n1, n3) < .8f);
	}
	{
		float grid1 = max(0.f, max((mod((ray.x+ray.y+ray.z*2.f)-time*3.f, 5.f)-4.f)*1.5f, 0.f) );
		float grid2 = max(0.f, max((mod((ray.x+ray.y*2.f+ray.z)-time*2.f, 7.f)-6.f)*1.2f, 0.f) );
		const auto gp1 = abs(mod(ray, vec3(.24f)));
		const auto gp2 = abs(mod(ray, vec3(.32f)));
		glow += grid1 * (!(gp1.x < .23f && gp1.z < .23f)) + grid2 * (!(gp2.y < .31f && gp2.z < .31f));
	}

	const float fog = float(march) / float(max_march);
	const auto  fog2 = .01f * vec3(1.f, 1.f, 1.5f) * total_d;
	glow *= min(1.f, 4.f - (4.f / float(max_march - 1)) * float(march));
	const float scanline = 1.f - .3f * (mod(coord.y, 4.f) < 2.f);
	const auto t = .15f + glow * .75f;
	return unorm4(vec3(t, t, .2f + glow) * fog + fog2, 1.f) * scanline;
}

};

namespace config{

static constexpr float time_per_frame = 0.1f;
static constexpr int width = 1920;
static constexpr int height = 1080;
static constexpr bool prerender = false;
using shader = i_saint_ray_marching;

}

}
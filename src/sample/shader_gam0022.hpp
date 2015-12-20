#pragma once
//http://qiita.com/gam0022/items/03699a07e4a4b5f2d41f

namespace amp_shader{

class reflect_ray_marching : public amp_shader_base<reflect_ray_marching>{

struct camera{
	float_3 get_side()const restrict(cpu, amp){return cross(dir, up);}
	float_3 pos;
	float_3 dir;
	float_3 up;
	__declspec(property(get = get_side)) float_3 side;
};

float_2 resolution;
float time;
const float EPS = 0.01f;
const float OFFSET = EPS * 100.f;
// normalize(vec3(-3, 5, -2))
const float_3 lightdir = vec3(-.48666426339228763f, .8111071056538127f, -.3244428422615251f);

// distance functions
float_3 on_rep(const float_3& p, float interval)const restrict(amp){
	auto q = mod(p.xz, interval) - interval * .5f;
	return vec3(q.x, p.y, q.y);
}

float sphere_dist(const float_3& p, float r)const restrict(amp){
	return length(on_rep(p, 3.f)) - r;
}

float floor_dist(const float_3& p)const restrict(amp){
	return dot(p, vec3(0.f, 1.f, 0.f)) + 1.f;
}

float_4 min_vec4(const float_4& a, const float_4& b)const restrict(amp){
	return (a.a < b.a) ? a : b;
}

float checkered_pattern(const float_3& p)const restrict(amp){
	const auto u = 1.f - floor(mod(p.x, 2.f));
	const auto v = 1.f - floor(mod(p.z, 2.f));
	if ((u == 1.f && v < 1.f) || (u < 1.f && v == 1.f))
		return .2f;
	else
		return 1.f;
}

// http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
float_3 hsv2rgb(const float_3& c)const restrict(amp){
	const auto K = vec4(1.f, 2.f / 3.f, 1.f / 3.f, 3.f);
	const auto p = abs(fract(vec3(c.x, c.x, c.x) + K.xyz) * 6.f - vec3(K.w, K.w, K.w));
	const auto Kxxx = vec3(K.x, K.x, K.x);
	return c.z * mix(Kxxx, clamp(p - Kxxx, 0.f, 1.f), c.y);
}

float scene_dist(const float_3& p)const restrict(amp){
	return min(
		sphere_dist(p, 1.f),
		floor_dist(p)
		);
}

float_4 scene_color(const float_3& p)const restrict(amp){
	return min_vec4(
		// 3 * 6 / 2 = 9
		vec4(hsv2rgb(vec3((p.z + p.x) / 9.f, 1.f, 1.f)), sphere_dist(p, 1.f)), 
		vec4(vec3(.5f) * checkered_pattern(p), floor_dist(p))
		);
}

float_3 gen_normal(const float_3& p)const restrict(amp){
	return normalize(vec3(
		scene_dist(p + vec3(EPS, 0.f, 0.f)) - scene_dist(p + vec3(-EPS,  0.f,  0.f)),
		scene_dist(p + vec3(0.f, EPS, 0.f)) - scene_dist(p + vec3( 0.f, -EPS,  0.f)),
		scene_dist(p + vec3(0.f, 0.f, EPS)) - scene_dist(p + vec3( 0.f,  0.f, -EPS))
		));
}

// http://wgld.org/d/glsl/g020.html
float gen_shadow(const float_3& ro, const float_3& rd)const restrict(amp){
	float h;
	float c = EPS;
	float r = 1.f;
	float shadow_coef = .5f;
	for (float t = 0.f; t < 50.f; ++t){
		h = scene_dist(ro + rd * c);
		if(h < EPS)
			return shadow_coef;
		r = min(r, h * 16.f / c);
		c += h;
	}
	return 1.f - shadow_coef + r * shadow_coef;
}

struct raycolor_result{
	float_3 p;
	float_3 normal;
	float_3 color;
	bool hit;
};
raycolor_result gen_raycolor(const float_3& origin, const float_3& ray)const restrict(amp){
	// marching loop
	float dist;
	float depth = 0.f;
	auto p = origin;
	for (int i = 0; i < 64; i++){
		dist = scene_dist(p);
		depth += dist;
		p = origin + depth * ray;
		if (abs(dist) < EPS)
			break;
	}

	// hit check and calc color
	if (abs(dist) < EPS) {
		const auto normal = gen_normal(p);
		const float diffuse = clamp(dot(lightdir, normal), .1f, 1.f);
		const float specular = pow(clamp(dot(reflect(lightdir, normal), ray), 0.f, 1.f), 10.f);
		const float shadow = gen_shadow(p + normal * OFFSET, lightdir);
		return {p, normal, (scene_color(p).rgb * diffuse + vec3(.8f) * specular) * max(.5f, shadow) - pow(clamp(.05f * depth, 0.f, .6f), 2.f), true};
	}
	else
		return {p, vec3(0.f), -pow(clamp(.05f * depth, 0.f, .6f), 2.f), false};
}

public:

reflect_ray_marching(const inputs& in):resolution(in.resolution), time(in.time){}

unorm_4 operator()(const float_2& coord)const restrict(amp){
	const float_2 pos = (coord.xy * 2.f - resolution) / min(resolution.x, resolution.y);
	camera cam = {vec3(0.f, .3f, time),
	              normalize(vec3(0.f, -.3f, 1.f)),
	              vec3(0.f)};
	cam.up = cross(cam.dir, vec3(1.f, 0.f ,0.f));
	const float focus = 1.3f;
	auto ray = normalize(cam.side * pos.x + cam.up * pos.y + cam.dir * focus);
	auto color = vec3(0.f);
	float alpha = 1.f;
	for (int i = 0; i < 3; ++i) {
		const auto res = gen_raycolor(cam.pos, ray);
		color += alpha * res.color;
		alpha *= .3f;
		ray = normalize(reflect(ray, res.normal));
		cam.pos = res.p + res.normal * OFFSET;
		if (!res.hit)
			break;
	}
	return vec4<unorm>(color, 1.f);
}

};

namespace config{

static constexpr float time_per_frame = 0.02f;
static constexpr unsigned int width = 512u;
static constexpr unsigned int height = width;
static constexpr bool prerender = false;
using shader = reflect_ray_marching;

}

}
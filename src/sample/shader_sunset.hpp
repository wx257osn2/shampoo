#pragma once
//https://www.shadertoy.com/view/XtSXzd

namespace amp_shader{

class sunset_ray_marching : public amp_shader_base<sunset_ray_marching>{

struct collide{
	float d;
	int type;
};

float_2 resolution;
float time;

float hash( float n )const restrict(amp){ return fract(sin(n)*753.5453123f); }
float noise( const float_3& x )const restrict(amp){
	const auto p = floor(x);
	auto f = fract(x);
	f = f * f * (3.f - 2.f * f);

	const float n = p.x + p.y * 157.f + 113.f * p.z;
	return mix(mix(mix( hash(n + 0.f), hash(n + 1.f),f.x),
		mix( hash(n+157.f), hash(n+158.f),f.x),f.y),
		mix(mix( hash(n+113.f), hash(n+114.f),f.x),
			mix( hash(n+270.f), hash(n+271.f),f.x),f.y),f.z);
}

collide distF(const float_3& p)const restrict(amp){
	float z=0.0;
	z += noise(vec3(p.xz,time*2.f));
	z += noise(vec3(p.xz,time+1.f/3.f));
	z += noise(vec3(p.xz,time*.5f+2.f/3.f));
	float planeD = p.y+1.f+z/8.f;
	float d = planeD;
	int t = 1;
	return {d, t};
}
float eps = 1e-2f;
float_3 normal(float_3 p)const restrict(amp){
	float_2 e = vec2(1.f, 0.f); // wow
	return normalize(vec3(
		distF(p+vec3(e.xy, e.y)).d-distF(p-vec3(e.xy, e.y)).d,
		distF(p+vec3(e.yx, e.y)).d-distF(p-vec3(e.yx, e.y)).d,
		distF(p+vec3(e.y, e.yx)).d-distF(p-vec3(e.y, e.yx)).d));
}
float_3 dome(float_3 v)const restrict(amp){
	if(v.x<0.f)v.x*=-1.f;
	v.y+=.05f;
	v = normalize(v);
	auto d = v;
	float e = dot(v,vec3(1.f,0.f,0.f));
	if(e>.95f)d = vec3(1.f,.5f,0.f);
	d += vec3(1.f,1.f,1.f)*pow(1.f-abs(e-.95f),13.f)*.8f;
	d += vec3(1.f,.5f,.5f)*pow(1.f-abs(v.y-.05f),70.f);
	return d;
}
float_3 render(const float_3& p, const float_3& v, const float_3& n, int t)const restrict(amp){
	if(t<1){
		return vec3(1.f,1.f,0.f);
	}else{
		auto def = vec3(0.f,0.f,5.f);
		def += vec3(1.f,1.f,1.f)*(p.y+2.f);
		def *= max(1.f,pow(p.x/5.f,.1f));
		auto rColor = dome(reflect(v,n));
		def *= rColor*.5f+.5f;
		return def;
	}
}
float_4 shoot(const float_3& p, float_3 v)const restrict(amp){
	collide dist;
	dist.type=-1;
	auto cur = p;
	v = normalize(v);
	float befR = -1.f, befDist = -1.f;
	bool maxIter=true;
	for(int i=0;i<50;i++){
		collide d = distF(cur);
		if(abs(d.d) < eps){
			dist = d;
			maxIter=false;
			break;
		}
		cur += d.d*v;
	}
	if(maxIter){
		float u = p.y-1.f;
		float len = u*v.x/v.y;
		if(len>0.f){
			len=sqrt(len*len+u*u);
			dist={len,1};
		}
	}
	if(dist.type<0)return vec4(dome(v),1.f);
	else return vec4(render(cur,v,normal(cur),dist.type),1.f);
}

public:

sunset_ray_marching(const inputs& in):resolution(in.resolution), time(in.time){}

unorm_4 operator()(const float_2& coord)const restrict(amp){
	const auto campos = vec3(0.f, 0.f, 0.f);
	const float_2 pos = (coord.xy * 2.f - resolution) / min(resolution.x, resolution.y);
	const auto raydir = vec3(1.f, pos.y, pos.x);
	return vec4<unorm>(shoot(campos, raydir));
}

};

namespace config{

static constexpr float time_per_frame = 0.02f;
static constexpr unsigned int width = 1280u;
static constexpr unsigned int height = 720u;
static constexpr bool prerender = false;
using shader = sunset_ray_marching;

}

}
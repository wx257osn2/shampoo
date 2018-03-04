#pragma once
//https://www.shadertoy.com/view/llBXDG

namespace amp_shader{

class moon_and_sky_ray_marching : public amp_shader_base<moon_and_sky_ray_marching>{

struct collide{
	float d;
	int type;
};

float_2 resolution;
float time;

float_3 nrand3( float_2 co )const restrict(amp){
	float_3 a = fract( cos( co.x*8.3e-3f + co.y )*vec3(1.3e5f, 4.7e5f, 2.9e5f) );
	float_3 b = fract( sin( co.x*0.3e-3f + co.y )*vec3(8.1e5f, 1.0e5f, 0.1e5f) );
	float_3 c = mix(a, b, .5f);
	return c;
}

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

float_3 m_product(const float_3& x)const restrict(amp){
	return vec3(              .8f  * x.y + .6f  * x.z,
	             -.8f * x.x + .36f * x.y - .48f * x.z,
	             -.6f * x.x - .48f * x.y + .64f * x.z);
}

float noiseF(float_3 p)const restrict(amp){
	float f = 0.f;
	float_3 q = 8.f*p;
	f  = 0.5000f*noise( q ); q = m_product(q)*2.01f;
	f += 0.2500f*noise( q ); q = m_product(q)*2.02f;
	f += 0.1250f*noise( q ); q = m_product(q)*2.03f;
	f += 0.0625f*noise( q ); q = m_product(q)*2.01f;
	return f;
}

float torus(float_3 p,float_2 t)const restrict(amp){
	float_2 q=vec2(length(p.xz)-t.x,p.y);
	return length(q)-t.y;
}
float smooth_floor(float x, float c)const restrict(amp){
	float a = fract(x);
	float b = floor(x);
	return ((pow(a,c)-pow(1.f-a,c))/2.f)+b;
}
collide distF(float_3 p)const restrict(amp){
	float tD = torus((p - vec3(300.f, -40.f, 200.f))*vec3(0.9f, 3.f, 1.f), vec2(150.f, 20.f));
	float tD2 = torus((p - vec3(500.f, -100.f, -400.f))*vec3(0.9f, 4.f, 1.f),vec2(200.f, 20.f));
	float field = length(p - vec3(300.f, -950.f, 0.f)) - 800.f - length(sin(normalize(p - vec3(300.f, -950.f, 0.f)) * 10.f)) * 20.f;
	float d = tD;
	d = min(d,tD2);
	d = min(d,field);
	int t=1;
	if(d==field)t=2;
	return {d,t};
}
const float eps = 1e-2f;
float_3 normal(float_3 p)const restrict(amp){
	float_2 e = vec2(1.f, 0.f); // wow
	return normalize(vec3(
		distF(p+vec3(e.xy, e.y)).d-distF(p-vec3(e.xy, e.y)).d,
		distF(p+vec3(e.yx, e.y)).d-distF(p-vec3(e.yx, e.y)).d,
		distF(p+vec3(e.y, e.yx)).d-distF(p-vec3(e.y, e.yx)).d));
}
float_3 dome(float_3 v)const restrict(amp){
	v.y*=-1.f;
	float_3 d=vec3(0.f, 0.4f,1.f);
	d*=pow(max(v.y+.7f, 0.f), 3.f)+0.5f;
	d.y-=0.4f*max(-(v.y-.5f),0.f);
	d+=noiseF(v)*pow(max(0.f,1.f-abs(v.y-.5f)),2.f);

	float_2 seed = v.zy * .6f;//https://www.shadertoy.com/view/MslGWN
	seed = floor(seed * resolution.x);
	float_3 rnd = nrand3( seed );
	float_4 starcolor = vec4(pow(rnd.y,40.f));
	d += pow(starcolor.x*5.f*(.8f-v.y),2.f);

	return d/5.f;
}
float_3 render(float_3 p,float_3 v,float_3 n,int t)const restrict(amp){
	if(t<2){
		float_3 b=(n.xzy+1.f)/2.f;
		b+=vec3(.5f,.7f,2.f)*(max(0.f,dot(n,v))+.5f);
		return b;
	}else{
		return (n+1.f)/2.f*vec3(.8f,1.f,1.5f)/1.4f*pow(max(0.f,dot(n,vec3(-1.2f,1.f,0.2f))),3.f)*(1.f-noiseF(p*.01f)/4.f)/1.5f;
	}
}
float_4 shoot(float_3 p,float_3 v)const restrict(amp){
	collide dist;
	dist.type=-1;
	float_3 cur = p;
	v = normalize(v);
	float befR = -1.0, befDist = -1.0;
	bool maxIter=true;
	for(int i=0;i<60;i++){
		collide d = distF(cur);
		if(abs(d.d) < eps){
			dist = d;
			maxIter=false;
			break;
		}
		cur += d.d*v;
	}
	if(dist.type<0)return vec4(dome(v),1.f);
	else return vec4(render(cur,v,normal(cur),dist.type),1.f);
}

float_3 moonF(float_2 p)const restrict(amp){
	float_3 c = vec3(0.f);
	float d = 0.4f-length(p);
	if(d>0.f){
		c = vec3(1.f);
		c -= vec3(1.4f,0.f,0.f)*smooth_floor((1.f-noiseF(vec3(p,9.5f)))*8.f,.3f)/8.f;
		c += pow(1.f-abs(d),10.f)/2.f;
	}
	c += max(0.f,pow(max(0.f,1.f-abs(d)),10.f))*vec3(1.f)/5.f;
	c += pow(max(0.f,0.5f-abs(d)),1.8f)*vec3(1.5f)/1.f;
	return c;
}

public:

moon_and_sky_ray_marching(const inputs& in):resolution(in.resolution), time(in.time){}

unorm_4 operator()(const float_2& coord)const restrict(amp){
	const auto cam_pos = vec3(0.f, 0.f, 0.f);
	const float_2 pos = (coord.xy * 2.f - resolution) / min(resolution.x, resolution.y);
	const auto raydir = vec3(1.f, pos.y, pos.x);
	float_4 col{0};
	for(float i = 0.f; i < 9.f; ++i)
		col += shoot(cam_pos, raydir + vec3(0.f, (mod(i, 3.f) - 1.f) * .5f / resolution.x, ((i / 3.f) - 1.f) * .5f / resolution.x));
	col/=9.f;
	float_4 moon = vec4(moonF(pos - vec2(0.f, .5f)), 1.f);
	return unorm4((col + moon).rgb, 1.f);
}

};

namespace config{

static constexpr float time_per_frame = 0.01f;
static constexpr int width = 1280;
static constexpr int height = 720;
static constexpr bool prerender = true;
using shader = moon_and_sky_ray_marching;

}

}
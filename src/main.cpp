#pragma warning(push)
#pragma warning(disable:5040)
#include<will/graphics.hpp>
#pragma warning(pop)
#include<will/window.hpp>
#include<will/dwm.hpp>

namespace amp_shader{
using namespace will::amp;
using namespace will::amp::shader;

struct inputs{
	texture_view<unorm_4, 2> texture;
	float_2 resolution;
	float time;
};

template<typename T>
struct amp_shader_base{
	static void apply_shader(const inputs& input, accelerator_view& accv){
		const T shader(input);
		auto texture = input.texture;
		const auto resolution = input.resolution;
		parallel_for_each(accv, texture.extent, [=](index<2> idx)restrict(amp){const auto coord = get_coord(idx, resolution); texture.set(idx, shader(coord).bgra);});
	}
};
}

#include"shader.hpp"

template<typename RenderTarget>
inline will::d2d::bitmap shader(RenderTarget&& rt, will::amp::graphics::texture_view<will::amp::graphics::unorm_4, 2>& tex_view, int width, int height, float t){
	namespace amp = will::amp;
	namespace ampg = will::amp::graphics;

	amp_shader::config::shader::apply_shader({tex_view, {static_cast<float>(width), static_cast<float>(height)}, t}, rt.get_accelerator_view());
	return +rt.reinterpret_convert<will::d2d::bitmap>(tex_view, will::d2d::bitmap::property{}.format(DXGI_FORMAT_B8G8R8A8_UNORM).alpha_mode(D2D1_ALPHA_MODE_PREMULTIPLIED));
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int showCmd)try{
	will::com_apartment com{will::com_apartment::thread::multi};
	auto wc =+ will::window::class_::register_(will::window::class_::property{}
	                   .background(CreateSolidBrush(0))
	                   .class_name(_T("Shampoo"))
	                   .instance_handle(hInst)
	                   .style(CS_HREDRAW | CS_VREDRAW));
	auto w =+ wc.create_window(will::window::property{}
	                   .title(_T("Shampoo"))
	                   .x(0).y(0)
	                   .w(500)
	                   .h(500));
	w.client_w = amp_shader::config::width;
	w.client_h = amp_shader::config::height;
	auto devcont =+ will::hwnd_render_target::create(w.get_hwnd());
	will::amp::graphics::texture<will::amp::graphics::unorm_4, 2> tex{amp_shader::config::height, amp_shader::config::width, 8u, devcont.get_accelerator_view()};
	will::amp::graphics::texture_view<will::amp::graphics::unorm_4, 2> tex_view{tex};
	float time = 0.f;
	auto texture = shader(devcont, tex_view, amp_shader::config::width, amp_shader::config::height, time);
	w.messenger[WM_ERASEBKGND] = [](auto&&, auto&&, auto&&)->LRESULT{return 0;};
	w.show();
	while(true){
		const auto start_point = std::chrono::steady_clock::now();
		time += amp_shader::config::time_per_frame;
		if(!amp_shader::config::prerender)
			texture = shader(devcont, tex_view, amp_shader::config::width, amp_shader::config::height, time);
		devcont.draw([&](auto&& x){
			x.clear(D2D1::ColorF(0, 0.f))
			 .image(texture);
		}).map([]{::DwmFlush();});
		if(auto ret = w.message_loop(start_point, 60); !ret)
			return ret.get();
	}
}catch(...){
	return will::window::exception_handler();
}

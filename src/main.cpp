#include<will/graphics.hpp>
#include<will/window.hpp>
#include<will/key.hpp>
#include<will/layered_window.hpp>
#include<will/dwm.hpp>
#include<will/amp.hpp>

namespace amp_shader{
using namespace will::amp;
using namespace will::ampm;
using namespace will::amp::shader;
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

inline will::d2d::bitmap shader(will::amp::accelerator_view& accv, will::ampg::texture_view<will::ampg::unorm_4, 2>& tex_view, will::d2d::device::context& devcont, will::d3d::device& dev, int width, int height, float t){
	namespace amp = will::amp;
	namespace ampm = will::ampm;
	namespace ampg = will::ampg;

	
	amp_shader::config::shader::apply_shader({tex_view, {static_cast<float>(width), static_cast<float>(height)}, t}, accv);
	auto tex_ = dev.create_texture2d(will::d3d::texture2d::description().width(width).height(height).format(DXGI_FORMAT_R8G8B8A8_UNORM).usage(D3D11_USAGE_STAGING).cpu_access_flags(D3D11_CPU_ACCESS_READ));
	auto d3ddevcont = dev.get_immediate_context();
	{
		will::d3d::texture2d tex_raw{reinterpret_cast<ID3D11Texture2D*>(ampg::direct3d::get_texture(tex_view))};
		d3ddevcont->CopyResource(tex_.get(), tex_raw.get());
		d3ddevcont->Flush();
	}
	auto subresource = d3ddevcont.create_scoped_subresource(tex_, 0, D3D11_MAP_READ);
	return will::d2d::bitmap{will::com_create_resource<ID2D1Bitmap1>([&](auto&& ptr){return devcont->CreateBitmap(D2D1::SizeU(width, height), subresource.pData, subresource.RowPitch, will::d2d::bitmap::property().format(DXGI_FORMAT_B8G8R8A8_UNORM).alpha_mode(D2D1_ALPHA_MODE_PREMULTIPLIED), ptr);})};
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int showCmd){
	will::com_apartment com{will::com_apartment::thread::multi};
	will::window w(std::move(will::window::window_class_property()
	                   .background(CreateSolidBrush(0))
	                   .class_name(_T("Shampoo"))
	                   .instance_handle(hInst)
	                   .style(CS_HREDRAW | CS_VREDRAW)),
	         std::move(will::window::window_property()
	                   .title(_T("Shampoo"))
	                   .x(0).y(0)
	                   .w(500)
	                   .h(500)));
	w.client_w = amp_shader::config::width;
	w.client_h = amp_shader::config::height;
	will::hwnd_render_target devcont(w.get_hwnd());
	auto accv = will::amp::direct3d::create_accelerator_view(devcont.get_d3d_device().get());
	will::ampg::texture<will::ampg::unorm_4, 2> tex(amp_shader::config::height, amp_shader::config::width, 8u, accv);
	will::ampg::texture_view<will::ampg::unorm_4, 2> tex_view(tex);
	float time = 0.f;
	auto texture = shader(accv, tex_view, devcont, devcont.get_d3d_device(), amp_shader::config::width, amp_shader::config::height, time);
	w.messenger[WM_ERASEBKGND] = [](auto&&, auto&&, auto&&)->LRESULT{return 0;};
	w.messenger[WM_PAINT] = [&](will::window& win, WPARAM, LPARAM)->LRESULT{
		if(!amp_shader::config::prerender)
			texture = shader(accv, tex_view, devcont, devcont.get_d3d_device(), amp_shader::config::width, amp_shader::config::height, time);
		devcont.draw([&](will::d2d::device::context& x){
			x->Clear(D2D1::ColorF(0, 0.f));
			x->DrawImage(texture.get());
		}, [](auto&&){::DwmFlush();});
		return 0;
	};
	w.show();
	will::window::message_process_result ret;
	do{
		const auto start_point = std::chrono::steady_clock::now();
		time += amp_shader::config::time_per_frame;
		ret = w.message_loop(start_point, 60);
	}while(ret);
	return 0;
}

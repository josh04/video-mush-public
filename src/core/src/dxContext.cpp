#include "dxContext.hpp"

#include <d3d9.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

#include <GL/glew.h>
#include <GL/wglew.h>

#include <azure/glexception.hpp>

namespace mush {
	dx::dx() {

	}

	dx::~dx() {

	}

	void dx::init(HWND hWndDX) {
		HRESULT hr;


		if (hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &_d3dContext)) {
			//DXTRACE_ERR_MSGBOX(NULL, hr);
			throw std::runtime_error("Direct3DCreate9Ex failed.");
		}
		// End Once

		bool fullscreen = false;

		D3DPRESENT_PARAMETERS d3dpp;

		ZeroMemory(&d3dpp, sizeof(d3dpp));
		//d3dpp.BackBufferCount = 1;
		d3dpp.Windowed = !fullscreen;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		d3dpp.hDeviceWindow = hWndDX;
		d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8; //D3DFMT_A2R10G10B10; //D3DFMT_A2R10G10B10; //D3DFMT_A2B10G10R10; //D3DFMT_X8R8G8B8;// D3DFMT_A16B16G16R16F; //D3DFMT_A32B32G32R32F
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		//d3dpp.EnableAutoDepthStencil = false;
		if (fullscreen) {
			d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			d3dpp.BackBufferWidth = 1920;
			d3dpp.BackBufferHeight = 1080;
		}
		//d3dpp.PresentationInterval = 0;

		D3DDISPLAYMODEEX fullscreenDisplayMode;
		ZeroMemory(&fullscreenDisplayMode, sizeof(fullscreenDisplayMode));
		fullscreenDisplayMode.Size = sizeof(D3DDISPLAYMODEEX);
		fullscreenDisplayMode.Width = d3dpp.BackBufferWidth;
		fullscreenDisplayMode.Height = d3dpp.BackBufferHeight;
		fullscreenDisplayMode.RefreshRate = d3dpp.FullScreen_RefreshRateInHz;
		fullscreenDisplayMode.Format = d3dpp.BackBufferFormat;
		fullscreenDisplayMode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

		if (hr = _d3dContext->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndDX, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, fullscreen ? &fullscreenDisplayMode : NULL, &_d3dDevice)) {
			//DXTRACE_ERR_MSGBOX(NULL, hr);
			throw std::runtime_error("CreateDeviceEx failed.");
		}


		/*glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		GLenum err2 = wglewInit();
		
		auto ptr = wglDXOpenDeviceNV;
		*/

		if (!(_deviceH = wglDXOpenDeviceNV(_d3dDevice))) {
			throw std::runtime_error("wglDXOpenDeviceNV failed.");
		}
	}

	IDirect3DSurface9 * dx::make_texture() {

		D3DSURFACE_DESC rtDesc;
		rtDesc.Format = D3DFMT_A8R8G8B8;
		rtDesc.Type = D3DRTYPE_TEXTURE;
		rtDesc.Height = 512;
		rtDesc.Width = 512;
		rtDesc.Usage = D3DUSAGE_RENDERTARGET;
		rtDesc.Pool = D3DPOOL_DEFAULT;
		rtDesc.MultiSampleQuality = 0;
		rtDesc.MultiSampleType = D3DMULTISAMPLE_NONE;
		/*
		if (HRESULT hr = _d3dDevice->CreateTexture(rtDesc.Width, rtDesc.Height, 1, D3DUSAGE_RENDERTARGET, rtDesc.Format, rtDesc.Pool, &_dx_texture, NULL)) {
			throw std::runtime_error("CreateTexture failed.");
		}
		*/
		HRESULT hr;
		HANDLE dx_share_handle = NULL;
		if (hr = _d3dDevice->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, rtDesc.MultiSampleType, 0, false, &_dx_texture2, &dx_share_handle)) {
			throw std::runtime_error("CreateRenderTarget Failed.");
		}
		wglDXSetResourceShareHandleNV(_dx_texture2, dx_share_handle);

		/*
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = 512;
		textureDesc.Height = 512;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		//textureDesc.Format = scd.BufferDesc.Format;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;

		if (HRESULT hr = _d3dDevice->CreateTexture2D(&textureDesc, NULL, &_dx_texture)) {
			throw std::runtime_error("CreateTexture2D failed.");
		}
		*/

		glGenTextures(1, &_gl_texture);
		_glException();

		glGenFramebuffers(1, &_gl_fbo);
		_glException();

		if (!(_dx_texture_handle = wglDXRegisterObjectNV(_deviceH, _dx_texture2, _gl_texture, GL_TEXTURE_2D, WGL_ACCESS_WRITE_DISCARD_NV))) {
			auto err = GetLastError();
			throw std::runtime_error("wglDXRegisterObjectNV failed.");
		}

		if (!wglDXLockObjectsNV(_deviceH, 1, &_dx_texture_handle)) {
			throw std::runtime_error("Failed to lock texture.");
		}

		glBindTexture(GL_TEXTURE_2D, _gl_texture);
		_glException();

		uint8_t * img = (uint8_t *)malloc(512 * 512 * 4 * 1);
		
		for (int j = 0; j < 512; ++j) {
			for (int i = 0; i < 512; ++i) {
				img[(j * 512 + i) * 4] = 255;
				img[(j * 512 + i) * 4 + 1] = 0;
				img[(j * 512 + i) * 4 + 2] = 0;
				img[(j * 512 + i) * 4 + 3] = 255;
			}
		}

		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, img);
		_glException();

		glBindTexture(GL_TEXTURE_2D, 0);
		_glException();

		if (!wglDXUnlockObjectsNV(_deviceH, 1, &_dx_texture_handle)) {
			throw std::runtime_error("Failed to unlock texture.");
		}
		

		return _dx_texture2;
	}
}

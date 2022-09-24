#ifndef DXCONTEXT_HPP
#define DXCONTEXT_HPP

#include <windows.h>

#include <d3d9.h>
#include <GL/glew.h>

#include "mush-core-dll.hpp"

struct IDirect3DTexture9;

namespace mush {
	class MUSHEXPORTS_API dx {
	public:
		dx();
		~dx();

		void init(HWND hWndDX);

		IDirect3DSurface9 * make_texture();

	private:
		IDirect3D9Ex * _d3dContext = NULL;
		IDirect3DDevice9Ex * _d3dDevice = NULL;

		HANDLE _deviceH = NULL;

		GLuint _gl_texture = 0;
		GLuint _gl_fbo = 0;

		IDirect3DSurface9 * _dx_texture2 = NULL;
		HANDLE _dx_texture_handle = NULL;


	};
}

#endif


#pragma once

#ifdef ES
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#include <GL/wglew.h>
namespace opengl {
	typedef HGLRC WINAPI Create_Context_Attribs_ARB(HDC, HGLRC, const int*);
	typedef HGLRC WINAPI Make_Current_ARB(HDC, HGLRC, const int*);

	inline static Create_Context_Attribs_ARB* create_context_arb{ nullptr };
}
#endif

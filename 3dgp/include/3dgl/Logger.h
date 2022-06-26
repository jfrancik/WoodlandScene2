#pragma once
/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

Warning and Error Message Codes and the Logger implementation
----------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

   Jarek Francik
   jarek@kingston.ac.uk
*********************************************************************************/
#ifndef __3dglLogger_h_
#define __3dglLogger_h_

#include <format>

// Include 3DGL API import/export settings
#include "3dglapi.h"

namespace _3dgl
{
	enum
	{
		_M3DGL_SUCCESS,
		M3DGL_SUCCESS_CREATED,
		M3DGL_SUCCESS_SRC_CODE_LOADED,
		M3DGL_SUCCESS_COMPILED,
		M3DGL_SUCCESS_LINKED,
		M3DGL_SUCCESS_ATTACHED,
		M3DGL_SUCCESS_ATTRIB_FOUND,
		M3DGL_SUCCESS_UNIFORM_FOUND,
		M3DGL_SUCCESS_VERIFICATION,
		M3DGL_SUCCESS_LOADED,
		M3DGL_SUCCESS_BONES_FOUND,
		M3DGL_SUCCESS_IMPORTING_FILE,

		_M3DGL_WARNING_GENERIC = 200,
		M3DGL_WARNING_UNIFORM_NOT_FOUND,
		M3DGL_WARNING_UNIFORM_NOT_REGISTERED,
		M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED,
		M3DGL_WARNING_NORMAL_COORDS_NOT_IMPLEMENTED,
		M3DGL_WARNING_VERTEX_BUFFER_MISSING,
		M3DGL_WARNING_NORMAL_BUFFER_MISSING,
		M3DGL_WARNING_TEXTURE_COORDS_BUFFER_MISSING,
		M3DGL_WARNING_COMPATIBLE_TEXTURE_COORDS_MISSING,
		M3DGL_WARNING_TANGENT_BUFFER_MISSING,
		M3DGL_WARNING_BITANGENT_BUFFER_MISSING,
		M3DGL_WARNING_COLOR_BUFFER_MISSING,
		M3DGL_WARNING_MAX_BONES_EXCEEDED,
		M3DGL_WARNING_BONE_WEIGHTS_NOT_1_0,
		M3DGL_WARNING_BONE_BUFFER_MISSING,
		M3DGL_WARNING_SKINNING_NOT_IMPLEMENTED,
		M3DGL_WARNING_CANNOT_LOAD_FROM,

		_M3DGL_ERROR_GENERIC = 500,
		M3DGL_ERROR_TYPE_MISMATCH,
		M3DGL_ERROR_AI,
		M3DGL_ERROR_COMPILATION,
		M3DGL_ERROR_LINKING,
		M3DGL_ERROR_WRONG_SHADER,
		M3DGL_ERROR_NO_SOURCE_CODE,
		M3DGL_ERROR_CREATION,
		M3DGL_ERROR_UNKNOWN_COMPILATION_ERROR,
		M3DGL_ERROR_SHADER_NOT_CREATED,
		M3DGL_ERROR_PROGRAM_NOT_CREATED,
		M3DGL_ERROR_UNKNOWN_LINKING_ERROR,
	};

	class MY3DGL_API CLogger
	{
	protected:
		CLogger();
		static bool _log(unsigned nCode, std::string name, std::string message);
	public:
		static CLogger& getInstance();
		std::string& operator[](const unsigned i);

		template<typename ... Args>
		static inline bool log(unsigned nCode, std::string name, Args... args)
		{
			return _log(nCode, name, std::vformat(getInstance()[nCode], std::make_format_args(args ...)));
		}
	};
}

#endif
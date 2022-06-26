/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

3dglTools: Simple tool functions
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

#ifndef __3dglTools_h_
#define __3dglTools_h_

#include "../glm/glm.hpp"
#include "../glm/gtc/constants.hpp"

// Include 3DGL API import/export settings
#include "3dglapi.h"

namespace _3dgl
{
	// pass the matrixView to get the current camera position
	glm::vec3 getPos(glm::mat4 m)
	{
		return glm::inverse(m)[3];
	}

	// pass the matrixView to get the current camera pitch
	float getPitch(glm::mat4 m)
	{
		float pi = glm::pi<float>();
		float half_pi = glm::half_pi<float>();
		return fmod(atan2(-m[2][1], m[2][2]) + pi + half_pi, pi) - half_pi;
	}

	// pass the matrixView to get the current camera yaw
	float getYaw(glm::mat4 m)
	{
		return atan2(m[2][0], m[0][0]);
	}

	// pass the matrixView to get the current camera roll
	float getRoll(glm::mat4 m)
	{
		return atan2(-m[1][0], m[1][1]);
	}
}; // namespace _3dgl

#endif
/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Partially based on http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html
Version 2.2 23/03/15

Copyright (C) 2013-15 Jarek Francik, Kingston University, London, UK

Implementation of a simple Material class
Uses AssImp (Open Asset Import Library) Library to interpret material information
Uses DevIL Image Library to load textures
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

#ifndef __3dglMaterial_h_
#define __3dglMaterial_h_

// Include GLM core features
#include "../glm/glm.hpp"

// Include 3DGL API import/export settings
#include "3dglapi.h"

struct aiMaterial;

namespace _3dgl
{
	struct MY3DGL_API C3dglMAT
	{
	private:
		// texture id
		unsigned m_idTexture[GL_TEXTURE31 - GL_TEXTURE0 + 1];

		// materials
		bool m_bAmb, m_bDiff, m_bSpec, m_bEmiss, m_bShininess;
		glm::vec3 m_amb, m_diff, m_spec, m_emiss;
		float m_shininess;

		static unsigned c_idTexBlank;

	public:
		C3dglMAT();
		void create(const aiMaterial* pMat, const char* pDefTexPath);
		void destroy();

		bool getAmbient(glm::vec3& val)		{ if (!m_bAmb) return false; val = m_amb; return true;  }
		bool getDiffuse(glm::vec3& val)		{ if (!m_bDiff) return false; val = m_diff; return true; }
		bool getSpecular(glm::vec3& val)	{ if (!m_bSpec) return false; val = m_spec; return true; }
		bool getEmissive(glm::vec3& val)	{ if (!m_bEmiss) return false; val = m_emiss; return true; }
		bool getShininess(float& val)		{ if (!m_bShininess) return false; val = m_shininess; return true; }

		bool getTexture(GLenum texUnit, unsigned& idTex) { unsigned i = m_idTexture[texUnit - GL_TEXTURE0];  if (i == 0xffffffff) return false; idTex = i; return true; }
		bool getTexture(unsigned& idTex)				 { return getTexture(GL_TEXTURE0, idTex); }


		void setAmbient(glm::vec3 colour)	{ m_bAmb = true; m_amb = colour; }
		void setDiffuse(glm::vec3 colour)	{ m_bDiff = true; m_diff = colour; }
		void setSpecular(glm::vec3 colour)	{ m_bSpec = true; m_spec = colour; }
		void setEmissive(glm::vec3 colour)	{ m_bEmiss = true; m_emiss = colour; }
		void setShininess(float s)			{ m_bShininess = true; m_shininess = s; }

		void loadTexture(GLenum texUnit, std::string strPath);
		void loadTexture(GLenum texUnit, std::string strTexRootPath, std::string strPath);
		void loadTexture(GLenum texUnit);

		void loadTexture(std::string strPath)								{ loadTexture(GL_TEXTURE0, strPath); }
		void loadTexture(std::string strTexRootPath, std::string strPath)	{ loadTexture(GL_TEXTURE0, strTexRootPath, strPath); }
		void loadTexture()													{ loadTexture(GL_TEXTURE0); }
	};
}

#endif
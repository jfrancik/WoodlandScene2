/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

Implementation of 3D mesh class
Partially based on http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html
Uses AssImp (Open Asset Import Library) Library to load model files
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

#ifndef __3dglMesh_h_
#define __3dglMesh_h_

// Include GLM core features
#include "../glm/glm.hpp"

// Include 3DGL API import/export settings
#include "3dglapi.h"
#include "CommonDef.h"

struct aiMesh;

namespace _3dgl
{
	class C3dglModel;
	struct C3dglMAT;

	struct MY3DGL_API C3dglMESH
	{
		// Owner
		C3dglModel *m_pOwner;

		// VAO (Vertex Array Object) id
		GLuint m_idVAO;

		// Buffers
		struct MY3DGL_API BUFFER
		{
			GLuint m_id;			// buffer id (as produced by glGenBuffers)
			void *m_pData;			// local copy of buffer data (only available if enabled in C3dglMesh::create)
			size_t m_num, m_size;	// parameters of the buffer data (only valid when m_pData available)

			BUFFER();

			void populate(size_t size, size_t num, const void* pData, bool bStoreLocally = false, GLenum target = GL_ARRAY_BUFFER, GLenum usage = GL_STATIC_DRAW);
			void getData(void** p, size_t& size, size_t& num);
			void release();
		} m_buf[ATTR_LAST], m_bufIndex;

		// number of elements to draw (size of index buffer)
		GLsizei m_indexSize;

		// number of texture UV coords (2 or 3 implemented)
		unsigned m_nUVComponents;

		// Material Index - points to the main m_materials collection
		size_t m_nMaterialIndex;
		
		// Bounding Box (experimental feature)
		glm::vec3 bb[2];
		glm::vec3 centre;

	public:
		C3dglMESH(C3dglModel* pOwner = NULL);

		void create(const aiMesh *pMesh, unsigned maskEnabledBufData = 0);
		void destroy();
		void render();

		C3dglMAT *getMaterial();
		C3dglMAT *createNewMaterial();

		// get buffer binary data - call C3dglModel::enableBufferData before loading!
		void getBufferData(ATTRIB_STD bufId, void **p, size_t &size, size_t &num)	{ m_buf[bufId].getData(p, size, num); }
		
		glm::vec3 *getBB()			{ return bb; }
		glm::vec3 getCentre()		{ return centre; }
	
		std::string getName();
	};

}; // namespace _3dgl

#endif
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

#include "Object.h"
#include "CommonDef.h"

struct aiMesh;

namespace _3dgl
{
	class C3dglModel;
	class C3dglMaterial;

	class MY3DGL_API C3dglMesh : public C3dglObject
	{
		// Owner
		C3dglModel *m_pOwner;

		// VAO (Vertex Array Object) id
		GLuint m_idVAO;

		// Attribute Buffer Ids
		GLuint m_id[ATTR_LAST];
		GLuint mIndexId;	// index buffer id

		// statistics
		unsigned m_nVertices;		// number of vertices
		unsigned m_nUVComponents;	// number of UV coords - must be 2
		unsigned m_indexSize;		// number of elements to draw (size of index buffer)
		unsigned m_nBones;			// number of bones

		// Material Index - points to the main m_materials collection
		size_t m_nMaterialIndex;
		
		// Bounding Volume (experimental feature)
		glm::vec3 m_aabb[2];
	
	protected:
		unsigned getBuffers(const aiMesh* pMesh, GLuint attribId[ATTR_LAST], void* attrData[ATTR_LAST], size_t attrSize[ATTR_LAST]);
		unsigned getIndexBuffer(const aiMesh* pMesh, void** indexData, size_t *indSize);
		void cleanUp(void** attrData, void *indexData);		// call after getBuffers well data no longer required
		void setupBoundingVolume(const aiMesh* pMesh);

	public:
		C3dglMesh(C3dglModel* pOwner);
		
		// Create a mesh using ASSIMP data and an array of Vertex Attributes (extracted from the current Shader Program)
		void create(const aiMesh *pMesh, GLuint attribId[ATTR_LAST]);

		// Create a mesh using ASSIMP data - to be used with the fixed pipeline only!
		void create(const aiMesh* pMesh);

		// Render the mesh
		void render();

		// Delete all buffer data - typically doesn't need to be called
		void destroy();

		// Using ASSIMP data, read attribute or index buffer data. A binary buffer will be allocated and a pointer stored in *ppData, 
		// *indSize will be filled with the element size and the function returns number of elements (0 if data unavailable).
		// The retuen value is also: number of vertices for getAttrData, number of indices for getIndexData.
		unsigned getAttrData(const aiMesh* pMesh, enum ATTRIB_STD attr, void** ppData, size_t* indSize);
		unsigned getIndexData(const aiMesh* pMesh, void** ppData, size_t* indSize);

		// Statistics
		unsigned getVertexCount()		{ return m_nVertices; }
		unsigned getUVComponentCount()	{ return m_nUVComponents; }
		unsigned getIndexCount()		{ return m_indexSize; }
		unsigned getBoneCount()			{ return m_nBones; }

		// Material functions
		C3dglMaterial *getMaterial();
		C3dglMaterial *createNewMaterial();

		// Bounding volume and geometrical centre
		void getAABB(glm::vec3 aabb[2]) { aabb[0] = m_aabb[0]; aabb[1] = m_aabb[1]; }
	
		std::string getName();
	};

}; // namespace _3dgl

#endif
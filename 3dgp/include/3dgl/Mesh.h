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
		// Owner and identification data name
		C3dglModel *m_pOwner;
		std::string m_name;
		const aiMesh *m_pMesh;

		// VAO (Vertex Array Object) id
		GLuint m_idVAO;

		// Attribute Buffer Ids
		static const size_t c_attrCount = ATTR_LAST;
		GLuint m_id[c_attrCount];
		GLuint m_idIndex;	// index buffer id

		// statistics
		size_t m_nVertices;			// number of vertices
		size_t m_nIndices;			// number of elements to draw (size of index buffer)
		size_t m_nBones;			// number of bones

		// Material Index - points to the main m_materials collection
		size_t m_matIndex;
		
		// Bounding Volume (experimental feature)
		glm::vec3 m_aabb[2];

		// Instancing values
		size_t m_instances = 1;
		GLuint m_divisor = 1;

	protected:
		size_t getBuffers(const aiMesh* pMesh, const GLint *attrId, void** attrData, size_t* attrSize, size_t attrCount) const;
		size_t getIndexBuffer(const aiMesh* pMesh, void** indexData, size_t *indSize) const;
		void cleanUp(void** attrData, void *indexData, size_t attrCount) const;		// call after getBuffers well data no longer required
		void getBoundingVolume(const aiMesh* pMesh, size_t nVertices, glm::vec3& aabb0, glm::vec3& aabb1) const;

	public:
		C3dglMesh(C3dglModel* pOwner = NULL);
		virtual ~C3dglMesh() { destroy(); }

		// Create a mesh using ASSIMP data and an array of Vertex Attributes (extracted from the current Shader Program)
		// For fixed pipeline use create(pMesh, NULL, 0);
		void create(const aiMesh* pMesh, const GLint* attrId, size_t attrCount);
		void create(const GLint* attrId, size_t attrCount, size_t nVertices, size_t nIndices, void** attrData, size_t* attrSize, void* indexData, size_t indSize);

		// Render the mesh
		void render() const;

		// Enable instancing by providing instanced data buffer (usually positionsl "offset" data)
		GLuint setupInstancingData(GLint attrLocation, size_t instances, GLint size, GLenum type, GLsizei stride, void* data, GLuint divisor = 1, GLenum usage = GL_STATIC_DRAW);
		void addInstancingData(GLint attrLocation, GLuint bufferId, GLint size, GLenum type, GLsizei stride, size_t offset);

		// Delete all buffer data - typically doesn't need to be called
		void destroy();

		// Using ASSIMP data, read attribute or index buffer data. A binary buffer will be allocated and a pointer stored in *ppData, 
		// *indSize will be filled with the element size and the function returns number of elements (0 if data unavailable).
		// The retuen value is also: number of vertices for getAttrData, number of indices for getIndexData.
		size_t getAttrData(enum ATTRIB_STD attr, void** ppData, size_t* indSize) const;
		size_t getIndexData(void** ppData, size_t* indSize) const;

		// Statistics
		size_t getVertexCount() const		{ return m_nVertices; }
		size_t getIndexCount() const		{ return m_nIndices; }
		size_t getBoneCount() const			{ return m_nBones; }

		// Material functions
		C3dglMaterial *getMaterial() const;
		C3dglMaterial *createNewMaterial();

		// Bounding volume and geometrical centre
		void getAABB(glm::vec3 aabb[2]) const	{ aabb[0] = m_aabb[0]; aabb[1] = m_aabb[1]; }
	
		std::string getName() const;
	};

}; // namespace _3dgl

#endif
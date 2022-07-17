/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

Implementation of generic renderable objects (parent for meshes, terrains and skyboxes)
Implements VAO (Vertex Attribute Object) with VBO (Vertex Buffers) 
and optional suppport for instancing
Partially based on http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html
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

#ifndef __3dglVertexAttrObject_h_
#define __3dglVertexAttrObject_h_

// Include GLM core features
#include "../glm/glm.hpp"

#include "Object.h"
#include "CommonDef.h"

namespace _3dgl
{
	class C3dglProgram;

	class MY3DGL_API C3dglVertexAttrObject : public C3dglObject
	{
		// VAO (Vertex Array Object) id
		GLuint m_idVAO = 0;

		// Attribute Buffers (VBO)
		size_t m_attrCount;			// number of different vertex attributes (or: buffer number)
		size_t m_nVertices = 0;		// number of vertices
		GLuint* m_idVBO;			// buffer id's (one buffer per vertex attribute)

		// Index Buffer
		size_t m_nIndices = 0;		// number of elements to draw (size of index buffer)
		GLuint m_idIndex = 0;		// index buffer id

		// Instancing values
		size_t m_instances = 1;

		// rendering-related data
		C3dglProgram* m_pProgram = NULL;					// program responsible for creating the VBO's and VAO; NULL if fixed pipeline or no VAO created
		mutable C3dglProgram* m_pLastProgramUsed = NULL;	// the last program used for rendering; NULL if never rendered since loading the model

	public:
		C3dglVertexAttrObject(size_t attrCount);
		C3dglVertexAttrObject(const C3dglVertexAttrObject&);		// copy constructor
		virtual ~C3dglVertexAttrObject();

		size_t getAttrCount() const						{ return m_attrCount; }

		GLuint getVAOid() const							{ return m_idVAO; }

		size_t getVertexCount() const					{ return m_nVertices; }
		GLuint getVertexBufferId(size_t attr) const		{ return m_idVBO[attr]; }

		size_t getIndexCount() const					{ return m_nIndices; }
		GLuint getIndexBufferId() const					{ return m_idIndex; }

		// The most generic create function
		void create(size_t attrCount, size_t nVertices, void** attrData, size_t* attrSize, size_t nIndices, void* indexData, size_t indSize, C3dglProgram* pProgram = NULL);

		// Render
		void render(glm::mat4 matrix, C3dglProgram* pProgram = NULL) const;
		virtual void render() const;

		// Delete all buffer data - typically doesn't need to be called
		virtual void destroy();

		// Enable instancing by providing instanced data buffer (usually positionsl "offset" data)
		GLuint setupInstancingData(GLint attrLocation, size_t instances, GLint size, GLenum type, GLsizei stride, void* data, GLuint divisor = 1, GLenum usage = GL_STATIC_DRAW);
		void addInstancingData(GLint attrLocation, GLuint bufferId, GLint size, GLenum type, GLsizei stride, size_t offset, GLuint divisor = 1);
		void destroyInstancingData(GLuint bufferId);
		size_t getInstancesCount() const				{ return m_instances; }

		using C3dglObject::getName;
	};

}; // namespace _3dgl

#endif
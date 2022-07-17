/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/VAO.h>
#include <3dgl/Shader.h>

// GLM include files
#include "../glm/gtc/type_ptr.hpp"

using namespace _3dgl;

/*********************************************************************************
** class C3dglVertexAttrObject
*/

C3dglVertexAttrObject::C3dglVertexAttrObject(size_t attrCount) : C3dglObject(), m_attrCount(attrCount)
{
	m_idVBO = new GLuint[m_attrCount];
	std::fill(m_idVBO, m_idVBO + m_attrCount, 0);
}

C3dglVertexAttrObject::C3dglVertexAttrObject(const C3dglVertexAttrObject& o) : C3dglObject(), m_attrCount(o.getAttrCount())
{
	m_idVBO = new GLuint[m_attrCount];
	std::copy(o.m_idVBO, o.m_idVBO + m_attrCount, m_idVBO);
}

C3dglVertexAttrObject::~C3dglVertexAttrObject() 
{ 
	destroy(); 
	delete[] m_idVBO;
}

void C3dglVertexAttrObject::create(size_t attrCount, size_t nVertices, void** attrData, size_t* attrSize, size_t nIndices, void* indexData, size_t indSize, C3dglProgram* pProgram)
{
	// Find the program to be used
	m_pProgram = pProgram;
	m_pLastProgramUsed = NULL;
	if (m_pProgram == NULL)
		m_pProgram = C3dglProgram::getCurrentProgram();

	// Aquire the Shader Signature - a collection of all standard attributes - see ATTRIB_STD enum for the list
	const GLint* attrId = NULL;
	if (m_pProgram)
	{
		attrId = m_pProgram->getShaderSignature();
		attrCount = glm::min(attrCount, m_pProgram->getShaderSignatureLength());

		// Generate warnings
		if (attrId[ATTR_VERTEX] == -1)
			log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
		if (attrId[ATTR_NORMAL] == -1)
			log(M3DGL_WARNING_NORMAL_COORDS_NOT_IMPLEMENTED);
		if (attrCount >= ATTR_COUNT && attrId[ATTR_BONE_ID] != -1 && attrId[ATTR_BONE_WEIGHT] == -1)
			log(M3DGL_WARNING_BONE_WEIGHTS_NOT_IMPLEMENTED);
		if (attrCount >= ATTR_COUNT && attrId[ATTR_BONE_ID] == -1 && attrId[ATTR_BONE_WEIGHT] != -1)
			log(M3DGL_WARNING_BONE_IDS_NOT_IMPLEMENTED);
	}
	else
		log(M3DGL_WARNING_NO_PROGRAMMABLE_PIPELINE);

	m_nVertices = nVertices;
	m_nIndices = nIndices;

	if (m_nVertices + m_nIndices == 0)
		return;			// nothing to do!

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// generate attribute buffers, then bind them and send data to OpenGL
	if (m_nVertices)
	{
		if (m_pProgram)
			// programmable pipeline
			for (unsigned attr = 0; attr < attrCount; attr++)
			{
				if (attrId[attr] == -1 || attrData[attr] == NULL)
					continue;
				glGenBuffers(1, &m_idVBO[attr]);
				glBindBuffer(GL_ARRAY_BUFFER, m_idVBO[attr]);
				glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);

				glEnableVertexAttribArray(attrId[attr]);
				static GLint attribMult[] = { 3, 3, 2, 3, 3, 3, MAX_BONES_PER_VERTEX, MAX_BONES_PER_VERTEX };
				if (attr == ATTR_BONE_ID)
					glVertexAttribIPointer(attrId[attr], attribMult[attr], GL_INT, 0, 0);
				else
					glVertexAttribPointer(attrId[attr], attribMult[attr], GL_FLOAT, GL_FALSE, 0, 0);
			}
		else
			// fixed pipeline only
			for (unsigned attr = 0; attr < ATTR_COUNT_BASIC; attr++)
			{
				if (attrData[attr] == NULL)
					continue;

				glGenBuffers(1, &m_idVBO[attr]);
				glBindBuffer(GL_ARRAY_BUFFER, m_idVBO[attr]);
				glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);

				switch (attr)
				{
				case ATTR_VERTEX: glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, 0); break;
				case ATTR_NORMAL: glEnableClientState(GL_NORMAL_ARRAY); glNormalPointer(GL_FLOAT, 0, 0); break;
				case ATTR_TEXCOORD: glEnableClientState(GL_TEXTURE_COORD_ARRAY); glTexCoordPointer(2, GL_FLOAT, 0, 0); break;
				}
			}
	}

	// Index buffer
	if (m_nIndices)
	{
		glGenBuffers(1, &m_idIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize * m_nIndices, indexData, GL_STATIC_DRAW);
	}

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void C3dglVertexAttrObject::render(glm::mat4 matrix, C3dglProgram* pProgram) const
{
	// check if a shading program is active
	if (pProgram == NULL)
		pProgram = C3dglProgram::getCurrentProgram();

	// perform compatibility checks
	// the only point of this paragraph is to display warnings if the shader used to render and the shader used to load the model are different
	if (pProgram != m_pLastProgramUsed)		// first conditional is used to bypass the check if the current program is the same as the last program used
	{
		m_pLastProgramUsed = pProgram;
		if (pProgram != m_pProgram)	// no checks needed if the current program is the same as the loading program
		{
			const GLint* pLoadSignature = m_pProgram->getShaderSignature();
			const GLint* pRenderSignature = pProgram->getShaderSignature();
			size_t nLoadLen = m_pProgram->getShaderSignatureLength();
			size_t nRenderLen = pProgram->getShaderSignatureLength();
			size_t nLen = glm::min(nLoadLen, nRenderLen);
			if (std::equal(pLoadSignature, pLoadSignature + nLen, pRenderSignature, [](GLint a, GLint b) { return a == -1 && b == -1 || a != -1 && b != -1; }))
				log(M3DGL_WARNING_DIFFERENT_PROGRAM_USED_BUT_COMPATIBLE);
			else
			{
				log(M3DGL_WARNING_INCOMPATIBLE_PROGRAM_USED);
				for (unsigned i = 0; i < nLen; i++)
					if (pLoadSignature[i] != -1 && pRenderSignature[i] == -1)
						log(M3DGL_WARNING_VERTEX_BUFFER_PREPARED_BUT_NOT_USED + i);
					else if (pLoadSignature[i] == -1 && pRenderSignature[i] != -1)
						log(M3DGL_WARNING_VERTEX_BUFFER_NOT_LOADED_BUT_REQUESTED + i);
			}
		}
	}

	// send model view matrix
	if (pProgram)
		pProgram->sendUniform(UNI_MODELVIEW, matrix);
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&matrix);
	}

	render();
}

void C3dglVertexAttrObject::render() const
{
	glBindVertexArray(m_idVAO);
	if (m_instances == 1)
		glDrawElements(GL_TRIANGLES, (GLsizei)m_nIndices, GL_UNSIGNED_INT, 0);
	else
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_nIndices, GL_UNSIGNED_INT, 0, (GLsizei)m_instances);
	glBindVertexArray(0);
}

void C3dglVertexAttrObject::destroy()
{
	for (unsigned attr = 0; attr < getAttrCount(); attr++)
		if (m_idVBO[attr] != 0)
			glDeleteBuffers(1, &m_idVBO[attr]);
	std::fill(m_idVBO, m_idVBO + getAttrCount(), 0);
	if (m_idIndex != 0) 
		glDeleteBuffers(1, &m_idIndex); 
	m_idIndex = 0;
	if (&m_idVAO != 0)
		glDeleteVertexArrays(1, &m_idVAO); 
	m_idVAO = 0;
	m_nVertices = m_nIndices = 0;
}

GLuint C3dglVertexAttrObject::setupInstancingData(GLint attrLocation, size_t instances, GLint size, GLenum type, GLsizei stride, void* data, GLuint divisor, GLenum usage)
{
	if (attrLocation == -1)
	{
		log(M3DGL_ERROR_INSTANCING_ATTR_NOT_FOUND);
		return 0;
	}
	if (m_instances != 1 && m_instances != instances)
	{
		log(M3DGL_ERROR_INSTANCING_CANNOT_BE_CHANGED, m_instances, instances);
		return 0;
	}

	m_instances = instances;

	if (stride == 0)
		switch (type)
		{
		case GL_FLOAT: stride = size * sizeof(float); break;
		case GL_INT:
		case GL_UNSIGNED_INT: stride = size * sizeof(float); break;
		}

	glBindVertexArray(m_idVAO);

	GLuint bufferId;

	glGenBuffers(1, &bufferId);
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferData(GL_ARRAY_BUFFER, instances * stride, data, usage);

	glEnableVertexAttribArray(attrLocation);
	glVertexAttribPointer(attrLocation, size, type, GL_FALSE, stride, 0);
	glVertexAttribDivisor(attrLocation, divisor);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return bufferId;
}

void C3dglVertexAttrObject::addInstancingData(GLint attrLocation, GLuint bufferId, GLint size, GLenum type, GLsizei stride, size_t offset, GLuint divisor)
{
	if (attrLocation == -1)
	{
		log(M3DGL_ERROR_INSTANCING_ATTR_NOT_FOUND);
		return;
	}

	glBindVertexArray(m_idVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);

	glEnableVertexAttribArray(attrLocation);
	glVertexAttribPointer(attrLocation, size, type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
	glVertexAttribDivisor(attrLocation, divisor);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void C3dglVertexAttrObject::destroyInstancingData(GLuint bufferId)
{
	glDeleteBuffers(1, &bufferId);
}



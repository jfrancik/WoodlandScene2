/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Mesh.h>
#include <3dgl/Model.h>

// assimp include file
#include "assimp/scene.h"

// GLM include files
#include "../glm/gtc/type_ptr.hpp"

using namespace _3dgl;

/*********************************************************************************
** struct C3dglMESH
*/

C3dglMesh::C3dglMesh(C3dglModel* pOwner) : m_pOwner(pOwner), m_pMesh(NULL), m_aabb{ glm::vec3(), glm::vec3() }, m_id{ 0, 0, 0, 0, 0, 0, 0, 0 }
{
	m_idVAO = 0;
	m_nVertices = m_nIndices = m_nBones = 0;
	m_idIndex = 0;
	m_matIndex = 0;
}

size_t C3dglMesh::getBuffers(const aiMesh* pMesh, const GLint* attrId, void** attrData, size_t* attrSize, size_t attrCount) const
{
	std::fill(attrData, attrData + attrCount, (void*)NULL);
	std::fill(attrSize, attrSize + attrCount, 0);

	// Some initial statistics
	size_t nVertices = pMesh->mNumVertices;				// number of vertices - must be > 0
	size_t nUVComponents = pMesh->mNumUVComponents[0];	// number of UV coords - must be 2
	size_t nBones = pMesh->mNumBones;					// number of bones (for skeletal animation)

	// Possible warnings...
	if (nVertices == 0) { log(M3DGL_WARNING_NO_VERTICES); return 0; }
	if (nUVComponents > 0 && nUVComponents != 2) log(M3DGL_WARNING_COMPATIBLE_TEXTURE_COORDS_MISSING, nUVComponents);

	// Convert texture coordinates to occupy contageous memory space
	GLfloat* pTexCoords = NULL;
	if (attrCount > ATTR_TEXCOORD && attrId[ATTR_TEXCOORD] != -1 && pMesh->mTextureCoords && pMesh->mTextureCoords[0])
	{
		pTexCoords = new GLfloat[nVertices * 2];
		for (size_t i = 0; i < nVertices; i++)
			memcpy(&pTexCoords[i * 2], &pMesh->mTextureCoords[0][i], sizeof(GLfloat) * 2);
	}

	// Create compatible Bone Id and Weight buffers - based on http://ogldev.atspace.co.uk/
	unsigned* pBoneIds = NULL;
	float* pBoneWeights = NULL;
	if (attrCount > ATTR_BONE_ID && attrId[ATTR_BONE_ID] != -1 && attrId[ATTR_BONE_WEIGHT] != -1 && nBones > 0)
	{
		pBoneIds = new unsigned[nVertices * MAX_BONES_PER_VERTEX];
		std::fill(pBoneIds, pBoneIds + nVertices * MAX_BONES_PER_VERTEX, 0);
		pBoneWeights = new float[nVertices * MAX_BONES_PER_VERTEX];
		std::fill(pBoneWeights, pBoneWeights + nVertices * MAX_BONES_PER_VERTEX, 0.0f);

		// for each bone:
		for (aiBone* pBone : std::vector<aiBone*>(pMesh->mBones, pMesh->mBones + pMesh->mNumBones))
		{
			// determine bone index from its name
			// if bone isn't yet known, it will be added
			size_t iBone = m_pOwner->getOrAddBone(pBone->mName.data, glm::transpose(glm::make_mat4((float*)&pBone->mOffsetMatrix)));

			// collect bone weights
			for (aiVertexWeight& weight : std::vector<aiVertexWeight>(pBone->mWeights, pBone->mWeights + pBone->mNumWeights))
			{
				// find a free location for the id and weight within bones[iVertex]
				unsigned i = 0;
				while (i < MAX_BONES_PER_VERTEX && pBoneWeights[weight.mVertexId * MAX_BONES_PER_VERTEX + i] != 0.0)
					i++;
				if (i < MAX_BONES_PER_VERTEX)
				{
					pBoneIds[weight.mVertexId * MAX_BONES_PER_VERTEX + i] = (unsigned)iBone;
					pBoneWeights[weight.mVertexId * MAX_BONES_PER_VERTEX + i] = weight.mWeight;
				}
				else
					log(M3DGL_WARNING_MAX_BONES_EXCEEDED);
			}
		}

		// normalise the weights
		for (size_t i = 0; i < nVertices * MAX_BONES_PER_VERTEX; i += MAX_BONES_PER_VERTEX)
		{
			float total = 0.0f;
			for (unsigned j = 0; j < MAX_BONES_PER_VERTEX; j++)
				total += pBoneWeights[i + j];
			for (unsigned j = 0; j < MAX_BONES_PER_VERTEX; j++)
				pBoneWeights[i + j] /= total;
		}
	}

	void* _data[] = { pMesh->mVertices, pMesh->mNormals, pTexCoords, pMesh->mTangents, pMesh->mBitangents, pMesh->mColors[0], pBoneIds, pBoneWeights };
	size_t _size[] = { sizeof(pMesh->mVertices[0]), sizeof(pMesh->mNormals[0]), sizeof(pTexCoords[0]) * nUVComponents,
		sizeof(pMesh->mTangents[0]), sizeof(pMesh->mBitangents[0]), sizeof(pMesh->mColors[0][0]), sizeof(pBoneIds[0]) * MAX_BONES_PER_VERTEX, sizeof(pBoneWeights[0]) * MAX_BONES_PER_VERTEX };

	// collect the actual buffer data
	for (unsigned attr = ATTR_VERTEX; attr < attrCount; attr++)
		if (attrId[attr] != -1)
		{
			if (!_data[attr])
				log(M3DGL_WARNING_VERTEX_BUFFER_MISSING + attr);
			attrData[attr] = _data[attr];
			attrSize[attr] = _size[attr];
		}

	return nVertices;
}

size_t C3dglMesh::getIndexBuffer(const aiMesh* pMesh, void** indexData, size_t* indSize) const
{
	*indexData = NULL;

	size_t nIndices = pMesh->mNumFaces;		// number of faces - must be > 0
	size_t nVertPerFace = pMesh->mFaces[0].mNumIndices;	// vertices per face - must be == 3

	// Possible warnings...
	if (nVertPerFace != 3) { log(M3DGL_WARNING_NON_TRIANGULAR_MESH); return 0; }

	unsigned *pIndices = new unsigned[nIndices * nVertPerFace];
	unsigned* p = pIndices;
	for (aiFace f : std::vector<aiFace>(pMesh->mFaces, pMesh->mFaces + nIndices))
		for (unsigned n : std::vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			*p++ = n;

	*indexData = pIndices;
	*indSize = sizeof(unsigned);
	return nIndices * nVertPerFace;
}

void C3dglMesh::cleanUp(void** attrData, void *indexData, size_t attrCount) const
{
	if (attrData && attrCount > ATTR_TEXCOORD && attrData[ATTR_TEXCOORD]) delete[] attrData[ATTR_TEXCOORD];
	if (attrData && attrCount > ATTR_BONE_ID && attrData[ATTR_BONE_ID]) delete[] attrData[ATTR_BONE_ID];
	if (attrData && attrCount > ATTR_BONE_WEIGHT && attrData[ATTR_BONE_WEIGHT]) delete[] attrData[ATTR_BONE_WEIGHT];
	if (indexData) delete[] indexData;
}

void C3dglMesh::getBoundingVolume(const aiMesh* pMesh, size_t nVertices, glm::vec3 &aabb0, glm::vec3& aabb1) const
{
	// find the BB (bounding box)
	aabb0 = aabb1 = glm::vec3(pMesh->mVertices[0].x, pMesh->mVertices[0].y, pMesh->mVertices[0].z);
	for (aiVector3D vec : std::vector<aiVector3D>(pMesh->mVertices, pMesh->mVertices + nVertices))
	{
		aabb0.x = std::min(aabb0.x, vec.x);
		aabb1.x = std::max(aabb1.x, vec.x);
		aabb0.y = std::min(aabb0.y, vec.y);
		aabb1.y = std::max(aabb1.y, vec.y);
		aabb0.z = std::min(aabb0.z, vec.z);
		aabb1.z = std::max(aabb1.z, vec.z);
	}
}

void C3dglMesh::create(const aiMesh* pMesh, const GLint* attrId, size_t attrCount)
{
	if (!pMesh) return;
	if (attrCount > c_attrCount)
		return;		// this should never happen!

	// attribute signature for fixed pipeline (count = 3)
	static GLint fixedAttr[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY };

	// collect buffered attribute data
	void* attrData[c_attrCount];
	size_t attrSize[c_attrCount];
	size_t nVertices = getBuffers(pMesh, attrId ? attrId : fixedAttr, attrData, attrSize, attrCount ? attrCount : 3);

	// collect index buffer data
	void* indexData;
	size_t indSize;
	size_t nIndices = getIndexBuffer(pMesh, &indexData, &indSize);

	create(attrId, attrCount, nVertices, nIndices, attrData, attrSize, indexData, indSize);

	// Additional data...
	getBoundingVolume(pMesh, nVertices, m_aabb[0], m_aabb[1]);
	m_matIndex = pMesh->mMaterialIndex;
	m_nBones = pMesh->mNumBones;
	m_pMesh = pMesh;
	m_name = pMesh->mName.data;
}

void C3dglMesh::create(const GLint* attrId, size_t attrCount, size_t nVertices, size_t nIndices, void** attrData, size_t* attrSize, void* indexData, size_t indSize)
{
	m_nVertices = nVertices;
	m_nIndices = nIndices;

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr < (attrCount ? attrCount : 3); attr++)
		if (attrData[attr])
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);

			if (attrCount > 0)
			{
				glEnableVertexAttribArray(attrId[attr]);
				static GLint attribMult[] = { 3, 3, 2, 3, 3, 3, MAX_BONES_PER_VERTEX, MAX_BONES_PER_VERTEX };
				if (attr == ATTR_BONE_ID)
					glVertexAttribIPointer(attrId[attr], attribMult[attr], GL_INT, 0, 0);
				else
					glVertexAttribPointer(attrId[attr], attribMult[attr], GL_FLOAT, GL_FALSE, 0, 0);
			}
			else
			{
				static GLint fixedAttr[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY };
				glEnableClientState(fixedAttr[attr]);
				switch (attr)
				{
				case ATTR_VERTEX: glVertexPointer(3, GL_FLOAT, 0, 0); break;
				case ATTR_NORMAL: glNormalPointer(GL_FLOAT, 0, 0); break;
				case ATTR_TEXCOORD: glTexCoordPointer(2, GL_FLOAT, 0, 0); break;
				}
			}
		}

	glGenBuffers(1, &m_idIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize * m_nIndices, indexData, GL_STATIC_DRAW);
	cleanUp(attrData, indexData, attrCount ? attrCount : 3);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void C3dglMesh::render() const
{
	glBindVertexArray(m_idVAO);
	if (m_instances == 1)
		glDrawElements(GL_TRIANGLES, (GLsizei)m_nIndices, GL_UNSIGNED_INT, 0);
	else
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_nIndices, GL_UNSIGNED_INT, 0, (GLsizei)m_instances);
	glBindVertexArray(0);
}

GLuint C3dglMesh::setupInstancingData(GLint attrLocation, size_t instances, GLint size, GLenum type, GLsizei stride, void* data, GLuint divisor, GLenum usage)
{
	m_instances = instances;
	m_divisor = divisor;

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

void C3dglMesh::addInstancingData(GLint attrLocation, GLuint bufferId, GLint size, GLenum type, GLsizei stride, size_t offset)
{
	glBindVertexArray(m_idVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);

	glEnableVertexAttribArray(attrLocation);
	glVertexAttribPointer(attrLocation, size, type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
	glVertexAttribDivisor(attrLocation, m_divisor);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void C3dglMesh::destroy()
{
	for (unsigned attr = ATTR_VERTEX; attr < c_attrCount; attr++)
		glDeleteBuffers(1, &m_id[attr]);
	std::fill(m_id, m_id + c_attrCount, 0);
	glDeleteBuffers(1, &m_idIndex); m_idIndex = 0;
	glDeleteVertexArrays(1, &m_idVAO); m_idVAO = 0;
}

size_t C3dglMesh::getAttrData(enum ATTRIB_STD attr, void** ppData, size_t* indSize) const
{
	*ppData = NULL;
	*indSize = NULL;
	if (!m_pMesh) return 0;

	GLint attrId[] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	attrId[attr] = 1;
	void* attrData[c_attrCount];
	size_t attrSize[c_attrCount];
	size_t nVertices = getBuffers(m_pMesh, attrId, attrData, attrSize, c_attrCount);
	*ppData = attrData[attr];
	*indSize = attrSize[attr];
	return nVertices;
}

size_t C3dglMesh::getIndexData(void** ppData, size_t* indSize) const
{
	*ppData = NULL;
	*indSize = NULL;
	if (!m_pMesh) return 0;

	return getIndexBuffer(m_pMesh, ppData, indSize);
}

C3dglMaterial* C3dglMesh::getMaterial() const
{
	return m_pOwner ? m_pOwner->getMaterial(m_matIndex) : NULL;
}

C3dglMaterial* C3dglMesh::createNewMaterial()
{
	if (!m_pOwner) return NULL;
	m_matIndex = m_pOwner->createNewMaterial();
	return getMaterial();
}

std::string C3dglMesh::getName() const
{
	if (!m_pOwner)
		return "Mesh";
	else if ((C3dglLogger::getOptions() & C3dglLogger::LOGGER_USE_MESH_NAMES) == 0)
		return std::format("{} mesh", m_pOwner->getName());
	else if (m_name.empty())
		return std::format("{}/mesh #{}", m_pOwner->getName(), m_pOwner->getMeshIndex(this));
	else
		return std::format("{}/mesh #{} ({})", m_pOwner->getName(), m_pOwner->getMeshIndex(this), m_name);
}

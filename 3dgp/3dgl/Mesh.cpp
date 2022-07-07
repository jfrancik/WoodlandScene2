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
	m_nVertices = m_nUVComponents = m_nIndices = m_nBones = 0;
	m_idIndex = 0;
	m_matIndex = 0;
}

size_t C3dglMesh::getBuffers(GLint attrId[ATTR_LAST], void* attrData[ATTR_LAST], size_t attrSize[ATTR_LAST])
{
	std::fill(attrData, attrData + ATTR_LAST, (void*)NULL);
	std::fill(attrSize, attrSize + ATTR_LAST, 0);

	// Some initial statistics
	m_nVertices = m_pMesh->mNumVertices;				// number of vertices - must be > 0
	m_nUVComponents = m_pMesh->mNumUVComponents[0];		// number of UV coords - must be 2
	m_nBones = m_pMesh->mNumBones;						// number of bones (for skeletal animation)

	// Possible warnings...
	if (m_nVertices == 0) { log(M3DGL_WARNING_NO_VERTICES); return 0; }
	if (m_nUVComponents > 0 && m_nUVComponents != 2) log(M3DGL_WARNING_COMPATIBLE_TEXTURE_COORDS_MISSING, m_nUVComponents);

	// Convert texture coordinates to occupy contageous memory space
	GLfloat* pTexCoords = NULL;
	if (attrId[ATTR_TEXCOORD] != -1 && m_nUVComponents == 2)
	{
		pTexCoords = new GLfloat[m_nVertices * 2];
		for (size_t i = 0; i < m_nVertices; i++)
			memcpy(&pTexCoords[i * 2], &m_pMesh->mTextureCoords[0][i], sizeof(GLfloat) * 2);
	}

	// Create compatible Bone Id and Weight buffers - based on http://ogldev.atspace.co.uk/
	unsigned* pBoneIds = NULL;
	float* pBoneWeights = NULL;
	if (attrId[ATTR_BONE_ID] != -1 && attrId[ATTR_BONE_WEIGHT] != -1 && m_nBones > 0)
	{
		pBoneIds = new unsigned[m_nVertices * MAX_BONES_PER_VERTEX];
		std::fill(pBoneIds, pBoneIds + m_nVertices * MAX_BONES_PER_VERTEX, 0);
		pBoneWeights = new float[m_nVertices * MAX_BONES_PER_VERTEX];
		std::fill(pBoneWeights, pBoneWeights + m_nVertices * MAX_BONES_PER_VERTEX, 0.0f);

		log(M3DGL_SUCCESS_BONES_FOUND, m_pMesh->mNumBones);

		// for each bone:
		for (aiBone* pBone : std::vector<aiBone*>(m_pMesh->mBones, m_pMesh->mBones + m_pMesh->mNumBones))
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
		for (size_t i = 0; i < m_nVertices * MAX_BONES_PER_VERTEX; i += MAX_BONES_PER_VERTEX)
		{
			float total = 0.0f;
			for (unsigned j = 0; j < MAX_BONES_PER_VERTEX; j++)
				total += pBoneWeights[i + j];
			for (unsigned j = 0; j < MAX_BONES_PER_VERTEX; j++)
				pBoneWeights[i + j] /= total;
		}
	}

	void* _data[] = { m_pMesh->mVertices, m_pMesh->mNormals, pTexCoords, m_pMesh->mTangents, m_pMesh->mBitangents, m_pMesh->mColors[0], pBoneIds, pBoneWeights };
	size_t _size[] = { sizeof(m_pMesh->mVertices[0]), sizeof(m_pMesh->mNormals[0]), sizeof(pTexCoords[0]) * m_nUVComponents,
		sizeof(m_pMesh->mTangents[0]), sizeof(m_pMesh->mBitangents[0]), sizeof(m_pMesh->mColors[0][0]), sizeof(pBoneIds[0]) * MAX_BONES_PER_VERTEX, sizeof(pBoneWeights[0]) * MAX_BONES_PER_VERTEX };

	// collect the actual buffer data
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		if (attrId[attr] != -1)
		{
			if (!_data[attr])
				log(M3DGL_WARNING_VERTEX_BUFFER_MISSING + attr);
			attrData[attr] = _data[attr];
			attrSize[attr] = _size[attr];
		}

	return m_nVertices;
}

size_t C3dglMesh::getIndexBuffer(void** indexData, size_t* indSize)
{
	*indexData = NULL;

	size_t nIndices = m_pMesh->mNumFaces;		// number of faces - must be > 0
	size_t nVertPerFace = m_pMesh->mFaces[0].mNumIndices;	// vertices per face - must be == 3

	// Possible warnings...
	if (nVertPerFace != 3) { log(M3DGL_WARNING_NON_TRIANGULAR_MESH); return 0; }

	unsigned *pIndices = new unsigned[nIndices * nVertPerFace];
	unsigned* p = pIndices;
	for (aiFace f : std::vector<aiFace>(m_pMesh->mFaces, m_pMesh->mFaces + nIndices))
		for (unsigned n : std::vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			*p++ = n;

	*indexData = pIndices;
	*indSize = sizeof(unsigned);
	return nIndices * nVertPerFace;
}

void C3dglMesh::cleanUp(void** attrData, void *indexData)
{
	if (attrData[ATTR_TEXCOORD]) delete[] attrData[ATTR_TEXCOORD];
	if (attrData[ATTR_BONE_ID]) delete[] attrData[ATTR_BONE_ID];
	if (attrData[ATTR_BONE_WEIGHT]) delete[] attrData[ATTR_BONE_WEIGHT];
	if (indexData) delete[] indexData;
}

void C3dglMesh::setupBoundingVolume()
{
	// find the BB (bounding box)
	m_aabb[0] = m_aabb[1] = glm::vec3(m_pMesh->mVertices[0].x, m_pMesh->mVertices[0].y, m_pMesh->mVertices[0].z);
	for (aiVector3D vec : std::vector<aiVector3D>(m_pMesh->mVertices, m_pMesh->mVertices + m_nVertices))
	{
		m_aabb[0].x = std::min(m_aabb[0].x, vec.x);
		m_aabb[1].x = std::max(m_aabb[1].x, vec.x);
		m_aabb[0].y = std::min(m_aabb[0].y, vec.y);
		m_aabb[1].y = std::max(m_aabb[1].y, vec.y);
		m_aabb[0].z = std::min(m_aabb[0].z, vec.z);
		m_aabb[1].z = std::max(m_aabb[1].z, vec.z);
	}
}

void C3dglMesh::create(const aiMesh* pMesh, GLint attrId[ATTR_LAST])
{
	if (!pMesh) return;

	// Some initial statistics
	m_pMesh = pMesh;									// aiMesg data for later reference
	m_name = pMesh->mName.data;							// mesh name

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// collect buffered attribute data
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	m_nVertices = getBuffers(attrId, attrData, attrSize);

	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		if (attrData[attr])
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * m_nVertices, attrData[attr], GL_STATIC_DRAW);
			
			glEnableVertexAttribArray(attrId[attr]);
			static GLint attribMult[] = { 3, 3, 2, 3, 3, 3, MAX_BONES_PER_VERTEX, MAX_BONES_PER_VERTEX };
			if (attr == ATTR_BONE_ID)
				glVertexAttribIPointer(attrId[attr], attribMult[attr], GL_INT, 0, 0);
			else
				glVertexAttribPointer(attrId[attr], attribMult[attr], GL_FLOAT, GL_FALSE, 0, 0);
		}

	// generate the index buffer, then bind it and send data to OpenGL
	void* pIndData;
	size_t indSize;
	m_nIndices = getIndexBuffer(&pIndData, &indSize);

	glGenBuffers(1, &m_idIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize * m_nIndices, pIndData, GL_STATIC_DRAW);
	cleanUp(attrData, pIndData);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Material information
	m_matIndex = m_pMesh->mMaterialIndex;

	// find the BB (bounding box)
	setupBoundingVolume();
}

void C3dglMesh::create(const aiMesh* pMesh)
{
	if (!pMesh) return;

	// Some initial statistics
	m_pMesh = pMesh;									// aiMesg data for later reference
	m_name = m_pMesh->mName.data;						// mesh name

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// collect buffered attribute data
	GLint attrId[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, -1, -1, -1, -1, -1 };
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	m_nVertices = getBuffers(attrId, attrData, attrSize);

	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr <= ATTR_TEXCOORD; attr++)
		if (attrData[attr])
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * m_nVertices, attrData[attr], GL_STATIC_DRAW);

			glEnableClientState(attrId[attr]);
			switch (attr)
			{
			case ATTR_VERTEX: glVertexPointer(3, GL_FLOAT, 0, 0); break;
			case ATTR_NORMAL: glNormalPointer(GL_FLOAT, 0, 0); break;
			case ATTR_TEXCOORD: glTexCoordPointer(2, GL_FLOAT, 0, 0); break;
			}
		}

	// first, convert indices to occupy contageous memory space
	std::vector<unsigned> indices;
	for (aiFace f : std::vector<aiFace>(m_pMesh->mFaces, m_pMesh->mFaces + m_pMesh->mNumFaces))
		for (unsigned n : std::vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			indices.push_back(n);

	// generate indices buffer, than bind it and send data to OpenGL
	glGenBuffers(1, &m_idIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);

	m_nIndices = indices.size();
	m_matIndex = m_pMesh->mMaterialIndex;

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// find the BB (bounding box)
	setupBoundingVolume();
}

void C3dglMesh::render()
{
	glBindVertexArray(m_idVAO);
	glDrawElements(GL_TRIANGLES, (GLsizei)m_nIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void C3dglMesh::destroy()
{
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		glDeleteBuffers(1, &m_id[attr]);
	std::fill(m_id, m_id + ATTR_VERTEX, 0);
	glDeleteBuffers(1, &m_idIndex); m_idIndex = 0;
	glDeleteVertexArrays(1, &m_idVAO); m_idVAO = 0;
}

size_t C3dglMesh::getAttrData(enum ATTRIB_STD attr, void** ppData, size_t* indSize)
{
	*ppData = NULL;
	*indSize = NULL;
	if (!m_pMesh) return 0;

	GLint attrId[] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	attrId[attr] = 1;
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	m_nVertices = getBuffers(attrId, attrData, attrSize);
	*ppData = attrData[attr];
	*indSize = attrSize[attr];
	return m_nVertices;
}

size_t C3dglMesh::getIndexData(void** ppData, size_t* indSize)
{
	*ppData = NULL;
	*indSize = NULL;
	if (!m_pMesh) return 0;

	return getIndexBuffer(ppData, indSize);
}

C3dglMaterial* C3dglMesh::getMaterial()
{
	return m_pOwner ? m_pOwner->getMaterial(m_matIndex) : NULL;
}

C3dglMaterial* C3dglMesh::createNewMaterial()
{
	m_matIndex = m_pOwner->createNewMaterial();
	return getMaterial();
}

std::string C3dglMesh::getName()
{
	if (m_name.empty())
		return std::format("{}/mesh #{}", m_pOwner->getName(), m_pOwner->getMeshIndex(this));
	else
		return std::format("{}/mesh #{} ({})", m_pOwner->getName(), m_pOwner->getMeshIndex(this), m_name);
}

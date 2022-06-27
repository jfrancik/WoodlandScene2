/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Mesh.h>
#include <3dgl/Model.h>

using namespace std;
using namespace _3dgl;

/*********************************************************************************
** struct C3dglMESH
*/

C3dglMESH::C3dglMESH(C3dglModel* pOwner) : m_pOwner(pOwner), m_bound1(), m_bound2(), m_centre(), m_id{ 0, 0, 0, 0, 0, 0, 0, 0 }
{
	m_idVAO = 0;
	m_indexSize = 0;
	mIndexId = 0;
	m_nMaterialIndex = 0;
}

unsigned C3dglMESH::getBuffers(const aiMesh* pMesh, GLuint attrId[ATTR_LAST], void* attrData[ATTR_LAST], size_t attrSize[ATTR_LAST])
{
	fill(attrData, attrData + ATTR_LAST, (void*)NULL);
	fill(attrSize, attrSize + ATTR_LAST, 0);

	// Some initial statistics
	unsigned nVertices = pMesh->mNumVertices;				// number of vertices - must be > 0
	unsigned nUVComponents = pMesh->mNumUVComponents[0];	// number of UV coords - must be 2
	unsigned nBones = pMesh->mNumBones;						// number of bones (for skeletal animation)

	// Possible warnings...
	if (nVertices == 0) { m_pOwner->log(M3DGL_WARNING_NO_VERTICES); return 0; }
	if (nUVComponents > 0 && nUVComponents != 2) m_pOwner->log(M3DGL_WARNING_COMPATIBLE_TEXTURE_COORDS_MISSING, nUVComponents);

	// Convert texture coordinates to occupy contageous memory space
	GLfloat* pTexCoords = NULL;
	if (attrId[ATTR_TEXCOORD] != (GLuint)-1 && nUVComponents == 2)
	{
		pTexCoords = new GLfloat[nVertices * 2];
		for (unsigned i = 0; i < nVertices; i++)
			memcpy(&pTexCoords[i * 2], &pMesh->mTextureCoords[0][i], sizeof(GLfloat) * 2);
	}

	// Create compatible Bone Id and Weight buffers - based on http://ogldev.atspace.co.uk/
	unsigned* pBoneIds = NULL;
	float* pBoneWeights = NULL;
	if (attrId[ATTR_BONE_ID] != (GLuint)-1 && attrId[ATTR_BONE_WEIGHT] != (GLuint)-1 && nBones > 0)
	{
		pBoneIds = new unsigned[nVertices * MAX_BONES_PER_VERTEX];
		fill(pBoneIds, pBoneIds + nVertices * MAX_BONES_PER_VERTEX, 0);
		pBoneWeights = new float[nVertices * MAX_BONES_PER_VERTEX];
		fill(pBoneWeights, pBoneWeights + nVertices * MAX_BONES_PER_VERTEX, 0.0f);

		m_pOwner->log(M3DGL_SUCCESS_BONES_FOUND, getName(), pMesh->mNumBones);

		// for each bone:
		for (aiBone* pBone : vector<aiBone*>(pMesh->mBones, pMesh->mBones + pMesh->mNumBones))
		{
			// determine bone index from its name
			size_t iBone;
			if (!m_pOwner->getOrAddBone(pBone->mName.data, iBone))
			{
				// only executed for new bones
				m_pOwner->m_vecBoneOffsets.push_back(pBone->mOffsetMatrix);
				m_pOwner->m_vecBoneNames.push_back(pBone->mName.data);
			}

			// collect bone weights
			for (aiVertexWeight& weight : vector<aiVertexWeight>(pBone->mWeights, pBone->mWeights + pBone->mNumWeights))
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
					m_pOwner->log(M3DGL_WARNING_MAX_BONES_EXCEEDED);
			}
		}

		// normalise the weights
		for (unsigned i = 0; i < nVertices * MAX_BONES_PER_VERTEX; i += MAX_BONES_PER_VERTEX)
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

	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		if (attrId[attr] != (GLuint)-1)
		{
			if (!_data[attr])
				m_pOwner->log(M3DGL_WARNING_VERTEX_BUFFER_MISSING + attr);
			attrData[attr] = _data[attr];
			attrSize[attr] = _size[attr];
		}

	return nVertices;
}

unsigned C3dglMESH::getIndexBuffer(const aiMesh* pMesh, void** indexData, size_t* indSize)
{
	*indexData = NULL;

	unsigned nIndices = pMesh->mNumFaces;		// number of faces - must be > 0
	unsigned nVertPerFace = pMesh->mFaces[0].mNumIndices;	// vertices per face - must be == 3

	// Possible warnings...
	if (nVertPerFace != 3) { m_pOwner->log(M3DGL_WARNING_NON_TRIANGULAR_MESH); return 0; }

	unsigned *pIndices = new unsigned[nIndices * nVertPerFace];
	unsigned* p = pIndices;
	for (aiFace f : vector<aiFace>(pMesh->mFaces, pMesh->mFaces + nIndices))
		for (unsigned n : vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			*p++ = n;

	*indexData = pIndices;
	*indSize = sizeof(unsigned);
	return nIndices * nVertPerFace;
}

void C3dglMESH::cleanUp(void** attrData, void *indexData)
{
	if (attrData[ATTR_TEXCOORD]) delete[] attrData[ATTR_TEXCOORD];
	if (attrData[ATTR_BONE_ID]) delete[] attrData[ATTR_BONE_ID];
	if (attrData[ATTR_BONE_WEIGHT]) delete[] attrData[ATTR_BONE_WEIGHT];
	if (indexData) delete[] indexData;
}

void C3dglMESH::setupBoundingVolume(const aiMesh* pMesh)
{
	// find the BB (bounding box)
	unsigned nVertices = pMesh->mNumVertices;
	m_bound1 = m_bound2 = *(glm::vec3*)(&pMesh->mVertices[0]);
	for (aiVector3D vec : vector<aiVector3D>(pMesh->mVertices, pMesh->mVertices + nVertices))
	{
		if (vec.x < m_bound1.x) m_bound1.x = vec.x;
		if (vec.y < m_bound1.y) m_bound1.y = vec.y;
		if (vec.z < m_bound1.z) m_bound1.z = vec.z;
		if (vec.x > m_bound2.x) m_bound2.x = vec.x;
		if (vec.y > m_bound2.y) m_bound2.y = vec.y;
		if (vec.z > m_bound2.z) m_bound2.z = vec.z;
	}
	m_centre.x = 0.5f * (m_bound1.x + m_bound2.x);
	m_centre.y = 0.5f * (m_bound1.y + m_bound2.y);
	m_centre.z = 0.5f * (m_bound1.z + m_bound2.z);
}

void C3dglMESH::create(const aiMesh* pMesh, GLuint attrId[ATTR_LAST])
{
	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// collect buffered attribute data
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	unsigned nVertices = getBuffers(pMesh, attrId, attrData, attrSize);

	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		if (attrData[attr])
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);
			
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
	m_indexSize = getIndexBuffer(pMesh, &pIndData, &indSize);

	glGenBuffers(1, &mIndexId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize * m_indexSize, pIndData, GL_STATIC_DRAW);
	cleanUp(attrData, pIndData);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Material information
	m_nMaterialIndex = pMesh->mMaterialIndex;

	// find the BB (bounding box)
	setupBoundingVolume(pMesh);
}

void C3dglMESH::create(const aiMesh* pMesh)
{
	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// collect buffered attribute data
	GLuint attrId[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1 };
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	unsigned nVertices = getBuffers(pMesh, attrId, attrData, attrSize);

	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr <= ATTR_TEXCOORD; attr++)
		if (attrData[attr])
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);

			glEnableClientState(attrId[attr]);
			switch (attr)
			{
			case ATTR_VERTEX: glVertexPointer(3, GL_FLOAT, 0, 0); break;
			case ATTR_NORMAL: glNormalPointer(GL_FLOAT, 0, 0); break;
			case ATTR_TEXCOORD: glTexCoordPointer(2, GL_FLOAT, 0, 0); break;
			}
		}

	// first, convert indices to occupy contageous memory space
	vector<unsigned> indices;
	for (aiFace f : vector<aiFace>(pMesh->mFaces, pMesh->mFaces + pMesh->mNumFaces))
		for (unsigned n : vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			indices.push_back(n);

	// generate indices buffer, than bind it and send data to OpenGL
	glGenBuffers(1, &mIndexId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);

	m_indexSize = (GLsizei)indices.size();
	m_nMaterialIndex = pMesh->mMaterialIndex;

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// find the BB (bounding box)
	setupBoundingVolume(pMesh);
}

void C3dglMESH::render()
{
	glBindVertexArray(m_idVAO);
	glDrawElements(GL_TRIANGLES, m_indexSize, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void C3dglMESH::destroy()
{
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_LAST; attr++)
		glDeleteBuffers(1, &m_id[attr]);
	fill(m_id, m_id + ATTR_VERTEX, 0);
	glDeleteBuffers(1, &mIndexId); mIndexId = 0;
	glDeleteVertexArrays(1, &m_idVAO); m_idVAO = 0;
}

unsigned C3dglMESH::getAttrData(const aiMesh* pMesh, enum ATTRIB_STD attr, void** ppData, size_t* indSize)
{
	GLuint attrId[] = { (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1 };
	attrId[attr] = 1;
	void* attrData[ATTR_LAST];
	size_t attrSize[ATTR_LAST];
	unsigned nVertices = getBuffers(pMesh, attrId, attrData, attrSize);
	*ppData = attrData[attr];
	*indSize = attrSize[attr];
	return nVertices;
}

unsigned C3dglMESH::getIndexData(const aiMesh* pMesh, void** ppData, size_t* indSize)
{
	return getIndexBuffer(pMesh, ppData, indSize);
}

C3dglMAT* C3dglMESH::getMaterial()
{
	return m_pOwner ? m_pOwner->getMaterial(m_nMaterialIndex) : NULL;
}

C3dglMAT* C3dglMESH::createNewMaterial()
{
	C3dglMAT mat;
	m_nMaterialIndex = m_pOwner->m_materials.size();
	m_pOwner->m_materials.push_back(mat);
	return getMaterial();
}

string C3dglMESH::getName()
{
	return std::string("mesh #") + std::to_string(this - &m_pOwner->m_meshes[0]);
}

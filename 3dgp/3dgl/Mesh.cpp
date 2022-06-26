/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Mesh.h>
#include <3dgl/Model.h>
#include <3dgl/Shader.h>

using namespace std;
using namespace _3dgl;

/*********************************************************************************
** struct C3dglMESH::BUFFER
*/

C3dglMESH::BUFFER::BUFFER()
{
	m_id = (unsigned)-1;
	m_pData = NULL;
	m_size = m_num = 0;
}

void C3dglMESH::BUFFER::populate(size_t size, size_t num, const void* pData, bool bStoreLocally, GLenum target, GLenum usage)
{
	glGenBuffers(1, &m_id);
	glBindBuffer(target, m_id);
	glBufferData(target, size * num, pData, usage);
	if (bStoreLocally && size * num == 0 && pData)
	{
		m_size = size;
		m_num = num;
		m_pData = new char[size * num];
		memcpy(m_pData, pData, m_size * m_num);
	}
}

void C3dglMESH::BUFFER::getData(void** p, size_t& size, size_t& num)
{
	if (p) *p = m_pData;
	size = m_size;
	num = m_num;
}

void C3dglMESH::BUFFER::release()
{
	glDeleteBuffers(1, &m_id);
	if (m_pData) delete[] m_pData;
	m_size = m_num = 0;
}

/*********************************************************************************
** struct C3dglMESH
*/

C3dglMESH::C3dglMESH(C3dglModel* pOwner) : m_pOwner(pOwner)
{
	m_idVAO = 0;
	m_indexSize = 0;
	m_nUVComponents = 0;
	m_nMaterialIndex = 0;
}

void C3dglMESH::create(const aiMesh* pMesh, unsigned maskEnabledBufData)
{
	if (pMesh->mFaces[0].mNumIndices != 3 && pMesh->mNumFaces && pMesh->mNumVertices && pMesh->mVertices && pMesh->mNormals)
		return;

	// find the BB (bounding box)
	bb[0] = bb[1] = *(glm::vec3*)(&pMesh->mVertices[0]);
	for (aiVector3D vec : vector<aiVector3D>(pMesh->mVertices, pMesh->mVertices + pMesh->mNumVertices))
	{
		if (vec.x < bb[0].x) bb[0].x = vec.x;
		if (vec.y < bb[0].y) bb[0].y = vec.y;
		if (vec.z < bb[0].z) bb[0].z = vec.z;
		if (vec.x > bb[0].x) bb[1].x = vec.x;
		if (vec.y > bb[0].y) bb[1].y = vec.y;
		if (vec.z > bb[0].z) bb[1].z = vec.z;
	}
	centre.x = 0.5f * (bb[0].x + bb[1].x);
	centre.y = 0.5f * (bb[0].y + bb[1].y);
	centre.z = 0.5f * (bb[0].z + bb[1].z);



	// Find the Current Program
	C3dglProgram* pProgram = C3dglProgram::GetCurrentProgram();

	// check shader parameters
	GLuint attribId[ATTR_LAST];
	for (unsigned attrib = ATTR_VERTEX; attrib < ATTR_LAST; attrib++)
		attribId[attrib] = pProgram ? pProgram->GetAttribLocation((ATTRIB_STD)attrib) : (GLuint)-1;

	// if no shader program in use...
	if (pProgram == NULL)
		attribId[ATTR_VERTEX] = attribId[ATTR_NORMAL] = attribId[ATTR_TEXCOORD] = 0;

	// Possible warnings...
	if (attribId[ATTR_VERTEX] == (GLuint)-1)
		m_pOwner->log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
	if (attribId[ATTR_NORMAL] == (GLuint)-1)
		m_pOwner->log(M3DGL_WARNING_NORMAL_COORDS_NOT_IMPLEMENTED);

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	unsigned attrib = ATTR_VERTEX;






	// generate a vertex buffer, than bind it and send data to OpenGL
	if (attribId[attrib] != (GLuint)-1)
		if (pMesh->mVertices)
		{
			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(pMesh->mVertices[0]), pMesh->mNumVertices, &pMesh->mVertices[0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], 3, GL_FLOAT, GL_FALSE, 0, 0);
			}
			else
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, 0);
			}
		}
		else
			m_pOwner->log(M3DGL_WARNING_VERTEX_BUFFER_MISSING);

	// generate a normal buffer, than bind it and send data to OpenGL
	attrib++;
	if (attribId[attrib] != (GLuint)-1)
		if (pMesh->mNormals)
		{
			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(pMesh->mNormals[0]), pMesh->mNumVertices, &pMesh->mNormals[0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], 3, GL_FLOAT, GL_FALSE, 0, 0);
			}
			else
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, 0);
			}
		}
		else
			m_pOwner->log(M3DGL_WARNING_NORMAL_BUFFER_MISSING);

	//Texture Coordinates
	attrib++;
	if (attribId[attrib] != (GLuint)-1)
	{
		m_nUVComponents = pMesh->mNumUVComponents[0];	// should be 2
		if (pMesh->mTextureCoords[0] == NULL)
			m_pOwner->log(M3DGL_WARNING_TEXTURE_COORDS_BUFFER_MISSING);
		else if (m_nUVComponents != 2 && m_nUVComponents != 3)
			m_pOwner->log(M3DGL_WARNING_COMPATIBLE_TEXTURE_COORDS_MISSING);
		else
		{
			// first, convert indices to occupy contageous memory space
			vector<GLfloat> texCoords;
			for (aiVector3D vec : vector<aiVector3D>(pMesh->mTextureCoords[0], pMesh->mTextureCoords[0] + pMesh->mNumVertices))
			{
				texCoords.push_back(vec.x);
				texCoords.push_back(vec.y);
				if (m_nUVComponents == 3)
					texCoords.push_back(vec.z);
			}

			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(texCoords[0]), texCoords.size(), &texCoords[0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], m_nUVComponents, GL_FLOAT, GL_FALSE, 0, 0);
			}
			else
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(m_nUVComponents, GL_FLOAT, 0, 0);
			}
		}
	}

	// generate a tangent buffer, than bind it and send data to OpenGL
	attrib++;
	if (attribId[attrib] != (GLuint)-1)
		if (pMesh->mTangents)
		{
			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(pMesh->mTangents[0]), pMesh->mNumVertices, &pMesh->mTangents[0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], 3, GL_FLOAT, GL_FALSE, 0, 0);
			}
		}
		else
			m_pOwner->log(M3DGL_WARNING_TANGENT_BUFFER_MISSING);

	// generate a biTangent buffer, than bind it and send data to OpenGL
	attrib++;
	if (attribId[attrib] != (GLuint)-1)
		if (pMesh->mBitangents)
		{
			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(pMesh->mBitangents[0]), pMesh->mNumVertices, &pMesh->mBitangents[0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], 3, GL_FLOAT, GL_FALSE, 0, 0);
			}
		}
		else
			m_pOwner->log(M3DGL_WARNING_BITANGENT_BUFFER_MISSING);

	// generate a color buffer, than bind it and send data to OpenGL
	attrib++;
	if (attribId[attrib] != (GLuint)-1)
		if (pMesh->mColors[0])
		{
			bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
			m_buf[attrib].populate(sizeof(pMesh->mColors[0][0]), pMesh->mNumVertices, &pMesh->mColors[0][0], bStoreLocally);

			if (pProgram)
			{
				glEnableVertexAttribArray(attribId[attrib]);
				glVertexAttribPointer(attribId[attrib], 3, GL_FLOAT, GL_FALSE, 0, 0);
			}
		}
		else
			m_pOwner->log(M3DGL_WARNING_COLOR_BUFFER_MISSING);

	// generate and convert a bone buffer, than bind it and send data to OpenGL
	attrib++;
	if (attribId[attrib] != (GLuint)-1 && attribId[attrib + 1] != (GLuint)-1)
	{
		// initislise the buffer
		struct VertexBoneData
		{
			unsigned ids[MAX_BONES_PER_VERTEX];
			float weights[MAX_BONES_PER_VERTEX];
		};
		vector<VertexBoneData> bones;
		bones.resize(pMesh->mNumVertices);
		memset(&bones[0], 0, sizeof(bones[0]) * bones.size());

		if (pMesh->mNumBones)
		{
			// load bone info - based on http://ogldev.atspace.co.uk/
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
					while (i < MAX_BONES_PER_VERTEX && bones[weight.mVertexId].weights[i] != 0.0)
						i++;
					if (i < MAX_BONES_PER_VERTEX)
					{
						bones[weight.mVertexId].ids[i] = (unsigned)iBone;
						bones[weight.mVertexId].weights[i] = weight.mWeight;
					}
					else
						m_pOwner->log(M3DGL_WARNING_MAX_BONES_EXCEEDED);
				}
			}

			// verify (and maybe, in future, normalize)
			bool bProblem = false;
			for (VertexBoneData& bone : bones)
			{
				float total = 0.0f;
				for (float weight : bone.weights)
					total += weight;
				bProblem = bProblem || total < 0.999f || total > 1.001f;
				//cout << total << endl;
			}
			if (bProblem)
				m_pOwner->log(M3DGL_WARNING_BONE_WEIGHTS_NOT_1_0);
		}
		else
			m_pOwner->log(M3DGL_WARNING_BONE_BUFFER_MISSING);

		bool bStoreLocally = (maskEnabledBufData & (1 << attrib)) != 0;
		m_buf[ATTR_BONE_ID].populate(sizeof(bones[0]), pMesh->mNumVertices, &bones[0], bStoreLocally);

		if (pProgram)
		{
			glEnableVertexAttribArray(attribId[ATTR_BONE_ID]);
			glVertexAttribIPointer(attribId[ATTR_BONE_ID], 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
			glEnableVertexAttribArray(attribId[ATTR_BONE_WEIGHT]);
			glVertexAttribPointer(attribId[ATTR_BONE_WEIGHT], 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)sizeof(bones[0].ids));
		}
	}

	// first, convert indices to occupy contageous memory space
	vector<unsigned> indices;
	for (aiFace f : vector<aiFace>(pMesh->mFaces, pMesh->mFaces + pMesh->mNumFaces))
		for (unsigned n : vector<unsigned>(f.mIndices, f.mIndices + f.mNumIndices))
			indices.push_back(n);

	// generate indices buffer, than bind it and send data to OpenGL
	m_bufIndex.populate(sizeof(indices[0]), indices.size(), &indices[0], false, GL_ELEMENT_ARRAY_BUFFER);
	///	if (maskEnabledBufData & (1 << ATTR_INDEX))
	///		m_bufIndex.storeData(sizeof(indices[0]), indices.size(), &indices[0]);
	/// 	NEEDS FURTHER WORK
	m_indexSize = (GLsizei)indices.size();

	m_nMaterialIndex = pMesh->mMaterialIndex;

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void C3dglMESH::destroy()
{
	m_buf[ATTR_VERTEX].release();
	m_buf[ATTR_NORMAL].release();
	m_buf[ATTR_TEXCOORD].release();
	m_buf[ATTR_TANGENT].release();
	m_buf[ATTR_BITANGENT].release();
	m_buf[ATTR_COLOR].release();
	m_buf[ATTR_BONE_ID].release();
	m_bufIndex.release();
}

void C3dglMESH::render()
{
	glBindVertexArray(m_idVAO);
	glDrawElements(GL_TRIANGLES, m_indexSize, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
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

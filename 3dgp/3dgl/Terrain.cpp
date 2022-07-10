/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <3dgl/CommonDef.h>
#include <3dgl/Bitmap.h>
#include <3dgl/Terrain.h>
#include <3dgl/Shader.h>

using namespace _3dgl;

C3dglTerrain::C3dglTerrain() : m_id{ 0, 0, 0, 0, 0 }, m_idIndex(0), m_idVAO(0)
{
	// height map
	m_heights = NULL;
    m_nSizeX = m_nSizeZ = 0;
	m_fScaleHeight = 1;
	m_pProgram = m_pLastProgramUsed = NULL;
}

void C3dglTerrain::createHeightMap(int nSizeX, int nSizeZ, float fScaleHeight, void* pBytes)
{
	m_nSizeX = nSizeX;
	m_nSizeZ = nSizeZ;
	m_fScaleHeight = fScaleHeight;

	// Collect Height Values
	m_heights = new float[m_nSizeX * m_nSizeZ];
	for (int i = 0; i < m_nSizeX; i++)
		for (int j = 0; j < m_nSizeZ; j++)
		{
			int index = (i + (m_nSizeZ - j - 1) * m_nSizeX) * 4;
			unsigned char val = static_cast<unsigned char*>(pBytes)[index];
			float f = (float)val / 256.0f;
			m_heights[i * m_nSizeX + j] = f * m_fScaleHeight;
		}
}

size_t C3dglTerrain::getBuffers(float* attrData[ATTR_COLOR], size_t attrSize[ATTR_COLOR])
{
	size_t nVertices = m_nSizeX * m_nSizeZ;
	GLint mul[] = { 3, 3, 2, 3, 3 };

	for (size_t attr = 0; attr < ATTR_COLOR; attr++)
	{
		attrData[attr] = new float[nVertices * mul[attr]];
		attrSize[attr] = sizeof(float) * mul[attr];
	}
	float* pVertex = attrData[0];
	float* pNormal = attrData[1];
	float* pTexCoord = attrData[2];
	float* pTangent = attrData[3];
	float* pBiTangent = attrData[4];

	int minx = -m_nSizeX / 2;
	int minz = -m_nSizeZ / 2;
	for (int x = minx; x < minx + m_nSizeX; x++)
		for (int z = minz; z < minz + m_nSizeZ; z++)
		{
			*pVertex++ = (float)x;
			*pVertex++ = getHeight(x, z);
			*pVertex++ = (float)z;

			int x0 = (x == minx) ? x : x - 1;
			int x1 = (x == minx + m_nSizeX - 1) ? x : x + 1;
			int z0 = (z == minz) ? z : z - 1;
			int z1 = (z == minz + m_nSizeZ - 1) ? z : z + 1;

			float dy_x = getHeight(x1, z) - getHeight(x0, z);
			float dy_z = getHeight(x, z1) - getHeight(x, z0);
			float m = sqrt(dy_x * dy_x + 4 + dy_z * dy_z);
			*pNormal++ = -dy_x / m;
			*pNormal++ = 2 / m;
			*pNormal++ = -dy_z / m;

			*pTexCoord++ = (float)x / 2.f;
			*pTexCoord++ = (float)z / 2.f;

			*pTangent++ = 1;
			*pTangent++ = dy_x / (x1 - x0);
			*pTangent++ = 0;

			*pBiTangent++ = 0;
			*pBiTangent++ = dy_z / (z1 - z0);
			*pBiTangent++ = 1;
		}

	return m_nSizeX * m_nSizeZ;
}

size_t C3dglTerrain::getIndexBuffer(GLuint** indexData, size_t* indSize)
{
	// Generate Indices

	/*
		We loop through building the triangles that
		make up each grid square in the heightmap

		(z*w+x) *----* (z*w+x+1)
				|   /|
				|  / |
				| /  |
	 ((z+1)*w+x)*----* ((z+1)*w+x+1)
	*/
	//Generate the triangle indices

	size_t indicesSize = (m_nSizeZ - 1) * (m_nSizeX - 1);
	GLuint* indices = new GLuint[indicesSize * 6], * pIndice = indices;
	for (int z = 0; z < m_nSizeZ - 1; ++z)
		for (int x = 0; x < m_nSizeX - 1; ++x)
		{
			*pIndice++ = x * m_nSizeZ + z; // current point
			*pIndice++ = x * m_nSizeZ + z + 1; // next row
			*pIndice++ = (x + 1) * m_nSizeZ + z; // same row, next col

			*pIndice++ = x * m_nSizeZ + z + 1; // next row
			*pIndice++ = (x + 1) * m_nSizeZ + z + 1; //next row, next col
			*pIndice++ = (x + 1) * m_nSizeZ + z; // same row, next col
		}

	*indexData = indices;
	*indSize = sizeof(GLuint);
	return indicesSize * 6;
}

void C3dglTerrain::cleanUp(float** attrData, GLuint* indexData)
{
	for (int attr = 0; attr < ATTR_COLOR; attr++)
		delete[] attrData[attr];
	delete[] indexData;
}


float C3dglTerrain::getHeight(int x, int z)
{
	x += m_nSizeX/2;
	z += m_nSizeZ/2;
	if (x < 0 || x >= m_nSizeX) return 0;
	if (z < 0 || z >= m_nSizeZ) return 0;
	return m_heights[x * m_nSizeZ + z];
}

	glm::vec3 barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c)
	{
		auto v0 = b - a;
		auto v1 = c - a;
		auto v2 = p - a;

		float d00 = glm::dot(v0, v0);
		float d01 = glm::dot(v0, v1);
		float d11 = glm::dot(v1, v1);
		float d20 = glm::dot(v2, v0);
		float d21 = glm::dot(v2, v1);
		float denom = d00 * d11 - d01 * d01;

		float v = (d11 * d20 - d01 * d21) / denom;
		float w = (d00 * d21 - d01 * d20) / denom;
		float u = 1.0f - v - w;

		return glm::vec3(u, v, w);
	}

float C3dglTerrain::getInterpolatedHeight(float fx, float fz)
{
	int x = (int)floor(fx);
	int z = (int)floor(fz);
	fx -= x;
	fz -= z;
	if (fx + fz < 1)
		return glm::dot(barycentric(glm::vec2(fx, fz), glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 0)), glm::vec3(getHeight(x, z), getHeight(x, z + 1), getHeight(x + 1, z)));
	else
		return glm::dot(barycentric(glm::vec2(fx, fz), glm::vec2(0, 1), glm::vec2(1, 0), glm::vec2(1, 1)), glm::vec3(getHeight(x, z + 1), getHeight(x + 1, z), getHeight(x + 1, z + 1)));
}

bool C3dglTerrain::load(const std::string filename, float scaleHeight)
{
	m_name = filename;
	size_t i = m_name.find_last_of("/\\");
	if (i != std::string::npos) m_name = m_name.substr(i + 1);
	i = m_name.find_last_of(".");
	if (i != std::string::npos) m_name = m_name.substr(0, i);

	C3dglBitmap bm;
	if (!bm.load(filename, GL_RGBA))
		return false;

	create(bm.getWidth(), abs(bm.getHeight()), scaleHeight, bm.getBits());
	return true;
}

void C3dglTerrain::create(int nSizeX, int nSizeZ, float fScaleHeight, void* pBytes)
{
	if (m_heights)
		destroy();

	// Find the Current Program
	m_pProgram = C3dglProgram::getCurrentProgram();
	m_pLastProgramUsed = NULL;

	// Aquire the Shader Signature - a collection of all standard attributes - see ATTRIB_STD enum for the list
	GLint* attrId = NULL;
	if (m_pProgram)
	{
		attrId = m_pProgram->getShaderSignature();

		// Generate warnings
		if (attrId[ATTR_VERTEX] == -1)
			log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
		if (attrId[ATTR_NORMAL] == -1)
			log(M3DGL_WARNING_NORMAL_COORDS_NOT_IMPLEMENTED);
	}
	else
	{
		log(M3DGL_WARNING_NO_PROGRAMMABLE_PIPELINE);
		static GLint a[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, -1, -1, -1, -1, -1 };
		attrId = a;
	}

	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	createHeightMap(nSizeX, nSizeZ, fScaleHeight, pBytes);

	// Prepare Attributes - and pack them into temporary buffers
	float* attrData[ATTR_COLOR];
	size_t attrSize[ATTR_COLOR];
	size_t nVertices = getBuffers(attrData, attrSize);

	// generate VBO's and enable attrinuttes in VAO
	for (size_t attr = ATTR_VERTEX; attr < ATTR_COLOR; attr++)
		if (attrId[attr] != -1)
		{
			// Prepare Vertex Buffer
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, nVertices * attrSize[attr], attrData[attr], GL_STATIC_DRAW);

			if (m_pProgram)
			{
				glEnableVertexAttribArray(attrId[attr]);
				glVertexAttribPointer(attrId[attr], static_cast<GLint>(attrSize[attr] / sizeof(float)), GL_FLOAT, GL_FALSE, 0, 0);
			}
			else
			{
				glEnableClientState(attrId[attr]);
				switch (attr)
				{
				case ATTR_VERTEX: glVertexPointer(3, GL_FLOAT, 0, 0); break;
				case ATTR_NORMAL: glNormalPointer(GL_FLOAT, 0, 0); break;
				case ATTR_TEXCOORD: glTexCoordPointer(2, GL_FLOAT, 0, 0); break;
				}
			}
		}

	GLuint* indices = NULL;
	size_t indSize = 0;
	size_t nIndices = getIndexBuffer(&indices, &indSize);

	glGenBuffers(1, &m_idIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize * nIndices, indices, GL_STATIC_DRAW);

	cleanUp(attrData, indices);

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void C3dglTerrain::destroy()
{
	for (unsigned attr = ATTR_VERTEX; attr < ATTR_COLOR; attr++)
		glDeleteBuffers(1, &m_id[attr]);
	std::fill(m_id, m_id + ATTR_COLOR, 0);
	glDeleteBuffers(1, &m_idIndex); m_idIndex = 0;
	glDeleteVertexArrays(1, &m_idVAO); m_idVAO = 0;
	delete[] m_heights;
	m_heights = NULL;
}

void C3dglTerrain::render(glm::mat4 matrix)
{
	// check if a shading program is active
	C3dglProgram* pProgram = C3dglProgram::getCurrentProgram();

	// perform compatibility checks
	// the only point of this paragraph is to display warnings if the shader used to render and the shader used to load the model are different
	if (pProgram != m_pLastProgramUsed)		// first conditional is used to bypass the check if the current program is the same as the last program used
	{
		m_pLastProgramUsed = pProgram;
		if (pProgram != m_pProgram)	// no checks needed if the current program is the same as the loading program
		{

			GLint* pLoadSignature = m_pProgram->getShaderSignature();
			GLint* pRenderSignature = pProgram->getShaderSignature();
			if (std::equal(pLoadSignature, pLoadSignature + ATTR_LAST, pRenderSignature, [](GLint a, GLint b) { return a == -1 && b == -1 || a != -1 && b != -1; }))
				log(M3DGL_WARNING_DIFFERENT_PROGRAM_USED_BUT_COMPATIBLE);
			else
			{
				log(M3DGL_WARNING_INCOMPATIBLE_PROGRAM_USED);
				for (unsigned i = 0; i < ATTR_LAST; i++)
					if (pLoadSignature[i] != -1 && pRenderSignature[i] == -1)
						log(M3DGL_WARNING_VERTEX_BUFFER_PREPARED_BUT_NOT_USED + i);
					else if (pLoadSignature[i] == -1 && pRenderSignature[i] != -1)
						log(M3DGL_WARNING_VERTEX_BUFFER_NOT_LOADED_BUT_REQUESTED + i);
			}
		}
	}

	if (pProgram)
		pProgram->sendUniform(UNI_MODELVIEW, matrix);
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&matrix);
	}

	glBindVertexArray(m_idVAO);
	glDrawElements(GL_TRIANGLES, (m_nSizeX - 1) * (m_nSizeZ - 1) * 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


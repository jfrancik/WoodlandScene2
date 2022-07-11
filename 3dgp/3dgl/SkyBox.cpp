/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <GL/glew.h>
#include <3dgl/Shader.h>
#include <3dgl/Bitmap.h>
#include <3dgl/SkyBox.h>
#include <3dgl/CommonDef.h>

using namespace _3dgl;

C3dglSkyBox::C3dglSkyBox() : m_id{0, 0, 0}
{
	m_idVAO = 0;
	std::fill(m_idTex, m_idTex + 6, 0);
}

bool C3dglSkyBox::load(const char* pFd, const char* pRt, const char* pBk, const char* pLt, const char* pUp, const char* pDn) 
{
	glGenTextures(6, m_idTex);

	// load six textures
	glActiveTexture(GL_TEXTURE0);
	const char*pFilenames[] = { pBk, pRt, pFd, pLt, pUp, pDn };
	for (int i = 0; i < 6; ++i)
	{
		C3dglBitmap bm(pFilenames[i], GL_RGBA);
		glGenTextures(1, &m_idTex[i]);
		glBindTexture(GL_TEXTURE_2D, m_idTex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), abs(bm.getHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());
	}

	float vertices[] = 
	{
		-1.0f,-1.0f,-1.0f,	 1.0f,-1.0f,-1.0f,	 1.0f, 1.0f,-1.0f,	-1.0f, 1.0f,-1.0f,	//Back
		-1.0f,-1.0f,-1.0f,	-1.0f, 1.0f,-1.0f,	-1.0f, 1.0f, 1.0f,	-1.0f,-1.0f, 1.0f,	//Left
		 1.0f,-1.0f, 1.0f,	-1.0f,-1.0f, 1.0f,	-1.0f, 1.0f, 1.0f,	 1.0f, 1.0f, 1.0f,	//Front
		 1.0f,-1.0f, 1.0f,	 1.0f, 1.0f, 1.0f,	 1.0f, 1.0f,-1.0f,	 1.0f,-1.0f,-1.0f,	//Right
		-1.0f, 1.0f, -1.0f,	 1.0f, 1.0f, -1.0f,	 1.0f, 1.0f,  1.0f,	-1.0f, 1.0f,  1.0f,	//Top
		 1.0f,-1.0f, -1.0f,	-1.0f,-1.0f, -1.0f,	-1.0f,-1.0f,  1.0f,	 1.0f,-1.0f,  1.0f	//Bottom
	};

	float normals[] = 
	{
		0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,			//Back
		1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,			//Left
		0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,		//Front
		-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,		//Right
		0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,		//Top
		0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0			//Bottom
	};

	float texCoords[] = 
	{
		0.0f, 0.0f,		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f,		//Back
		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f,		0.0f, 0.0f,		//Left
		0.0f, 0.0f,		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f,		//Front
		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f,		0.0f, 0.0f,		//Right
		0.0f, 0.0f,		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f,		//Top
		0.0f, 0.0f,		1.0f, 0.0f,		1.0f, 1.0f,		0.0f, 1.0f		//Bottom
	};

	float* attrData[] = { vertices, normals,  texCoords };
	size_t attrSize[] = { 3 * sizeof(float), 3 * sizeof(float), 2 * sizeof(float) };
	size_t nVertices = 24;


	// create VAO
	glGenVertexArrays(1, &m_idVAO);
	glBindVertexArray(m_idVAO);

	// Find the Current Program
	C3dglProgram* pProgram = C3dglProgram::getCurrentProgram();

	// Aquire the Shader Signature - a collection of all standard attributes - see ATTRIB_STD enum for the list
	GLint* attrId = NULL;
	if (pProgram)
	{
		attrId = pProgram->getShaderSignature();

		// Generate warnings
		if (attrId[ATTR_VERTEX] == -1)
			log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
		if (attrId[ATTR_TEXCOORD] == -1)
			log(M3DGL_WARNING_TEXCOORDS_COORDS_NOT_IMPLEMENTED);
	}
	else
	{
		log(M3DGL_WARNING_NO_PROGRAMMABLE_PIPELINE);
		static GLint a[] = { GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, -1, -1, -1, -1, -1 };
		attrId = a;
	}
	
	// generate attribute buffers, then bind them and send data to OpenGL
	for (unsigned attr = ATTR_VERTEX; attr < c_attrCount; attr++)
		if (attrId[attr] != -1)
		{
			glGenBuffers(1, &m_id[attr]);
			glBindBuffer(GL_ARRAY_BUFFER, m_id[attr]);
			glBufferData(GL_ARRAY_BUFFER, attrSize[attr] * nVertices, attrData[attr], GL_STATIC_DRAW);

			if (pProgram)
			{
				glEnableVertexAttribArray(attrId[attr]);
				static GLint attribMult[] = { 3, 3, 2 };
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

	// Reset VAO & buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return true;
}

void C3dglSkyBox::render(glm::mat4 matrix)
{
	// send model view matrix
	matrix[3][0] = matrix[3][1] = matrix[3][2] = 0;

	C3dglProgram* pProgram = C3dglProgram::getCurrentProgram();
	if (pProgram)
	{
		pProgram->sendUniform(UNI_MODELVIEW, matrix);
		if (pProgram->getAttribLocation(ATTR_VERTEX) == -1)
			log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
		if (pProgram->getAttribLocation(ATTR_TEXCOORD) == -1)
			log(M3DGL_WARNING_TEXCOORDS_COORDS_NOT_IMPLEMENTED);
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&matrix);
	}

	// disable depth-buffer write cycles - so that the skybox cannot obscure anything
	GLboolean bDepthMask;
	::glGetBooleanv(GL_DEPTH_WRITEMASK, &bDepthMask);
	glDepthMask(GL_FALSE);

	glBindVertexArray(m_idVAO);
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < 6; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, m_idTex[i]);
		glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
	}
	glBindVertexArray(0);

	// enable depth-buffer write cycle
	glDepthMask(bDepthMask);
}

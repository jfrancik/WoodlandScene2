/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <fstream>
#include <vector>

// GLM include files
#include "../glm/gtc/type_ptr.hpp"

#include <GL/glew.h>
#include <3dgl/CommonDef.h>
#include <3dgl/Bitmap.h>
#include <3dgl/Terrain.h>
#include <3dgl/Shader.h>
#include <3dgl/Mesh.h>

using namespace _3dgl;

bool MY3DGL_API _3dgl::convHeightmap2OBJ(const std::string fileImage, float scaleHeight, const std::string fileOBJ)
{
	C3dglBitmap bm;
	if (!bm.load(fileImage, GL_RGBA) || !bm.getBits())
		return false;

	C3dglTerrain terrain;
	terrain.createHeightMap(bm.getWidth(), abs(bm.getHeight()), scaleHeight, bm.getBits());

	// Prepare Attributes - and pack them into temporary buffers
	const size_t attrCount = ATTR_TANGENT;	// This is a OBJ file, no need to create more than 3 attrins as tangents and bitangents are not supported!
	float* attrData[attrCount];
	size_t attrSize[attrCount];
	size_t nVertices = terrain.getBuffers(attrCount, attrData, attrSize);

	GLuint* indices = NULL;
	size_t indSize = 0;
	size_t nIndices = terrain.getIndexBuffer(&indices, &indSize);

	// Writing part...
	std::ofstream wf(fileOBJ, std::ios::out);
	wf << "# 3dgl Mesh Exporter - (c)2021 Jarek Francik" << std::endl << std::endl;
	wf << "# object Terrain1" << std::endl << std::endl;
	
	
	for (size_t i = 0; i < nVertices; i++)
		wf << "v  " << attrData[ATTR_VERTEX][3 * i] << " " << attrData[ATTR_VERTEX][3 * i + 1] << " " << attrData[ATTR_VERTEX][3 * i + 2] << std::endl;
	wf << std::endl;
	
	for (size_t i = 0; i < nVertices; i ++)
		wf << "vn  " << attrData[ATTR_NORMAL][3 * i] << " " << attrData[ATTR_NORMAL][3 * i + 1] << " " << attrData[ATTR_NORMAL][3 * i + 2] << std::endl;
	wf << std::endl;
	
	for (size_t i = 0; i < nVertices; i++)
		wf << "vt  " << attrData[ATTR_TEXCOORD][2 * i] << " " << attrData[ATTR_TEXCOORD][2 * i + 1] << " " << 0.0f << std::endl;
	wf << std::endl;
	
	wf << "g Terrain1" << std::endl;
	wf << "s 1" << std::endl;

	for (size_t i = 0; i < nIndices; i += 3)
		wf << "f  " << indices[i]+1 << "/" << indices[i] + 1 << "/" << indices[i] + 1 << " "
					<< indices[i+1] + 1 << "/" << indices[i+1] + 1 << "/" << indices[i+1] + 1 << " "
					<< indices[i+2] + 1 << "/" << indices[i+2] + 1 << "/" << indices[i+2] + 1 << std::endl;

	terrain.cleanUp(attrCount, attrData, indices);

	return true;
}

bool MY3DGL_API _3dgl::convHeightmap2Mesh(const std::string fileImage, float scaleHeight, C3dglMesh& mesh, C3dglProgram* pProgram)
{
	C3dglBitmap bm;
	if (!bm.load(fileImage, GL_RGBA) || !bm.getBits())
		return false;

	C3dglTerrain terrain;
	terrain.createHeightMap(bm.getWidth(), abs(bm.getHeight()), scaleHeight, bm.getBits());

	// Prepare Attributes - and pack them into temporary buffers
	const size_t attrCount = ATTR_COLOR;	// 5 attribs but no colour and no bones
	float* attrData[attrCount];
	size_t attrSize[attrCount];
	size_t nVertices = terrain.getBuffers(attrCount, attrData, attrSize);

	GLuint* indices = NULL;
	size_t indSize = 0;
	size_t nIndices = terrain.getIndexBuffer(&indices, &indSize);

	static_cast<C3dglVertexAttrObject*>(&mesh)->create(attrCount, nVertices, (void**)attrData, attrSize, nIndices, indices, indSize, pProgram);

	terrain.cleanUp(attrCount, attrData, indices);

	return true;
}

/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Model.h>
#include <3dgl/Shader.h>

// assimp include file
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <assimp/cimport.h>
#include <assimp/DefaultLogger.hpp>

// GLM include files
#include "../glm/vec3.hpp"
#include "../glm/vec4.hpp"
#include "../glm/mat4x4.hpp"
#include "../glm/gtc/type_ptr.hpp"

using namespace std;
using namespace _3dgl;

const GLuint NEG1 = (GLuint)-1;

C3dglModel::C3dglModel() : C3dglObject(), m_attribId{ NEG1, NEG1, NEG1, NEG1,NEG1, NEG1,NEG1, NEG1 }, m_globInvT(1)
{ 
	m_pScene = NULL; 
	m_pProgram = 0;
}

void C3dglModel::enableAssimpLoggingLevel(AssimpLoggingLevel level)
{
	Assimp::DefaultLogger::create("", (Assimp::Logger::LogSeverity)level, aiDefaultLogStream_STDOUT);
}

void C3dglModel::disableAssimpLoggingLevel()
{
	Assimp::DefaultLogger::kill();
}

bool C3dglModel::load(const char* pFile, unsigned int flags)
{
	if (flags == 0)
		flags = aiProcessPreset_TargetRealtime_MaxQuality;

	log(M3DGL_SUCCESS_IMPORTING_FILE, pFile);
	m_name = pFile;
	size_t i = m_name.find_last_of("/\\");
	if (i != string::npos) m_name = m_name.substr(i + 1);
	i = m_name.find_last_of(".");
	if (i != string::npos) m_name = m_name.substr(0, i);

	const aiScene* pScene = aiImportFile(pFile, flags);
	if (pScene == NULL)
		return log(M3DGL_ERROR_AI, aiGetErrorString());
	create(pScene);
	return true;
}

void C3dglModel::create(const aiScene* pScene)
{
	// Find the Current Program
	m_pProgram = C3dglProgram::GetCurrentProgram();

	// Store the shader program attribute values
	if (m_pProgram)
		for (unsigned attrib = ATTR_VERTEX; attrib < ATTR_LAST; attrib++)
			m_attribId[attrib] = m_pProgram->GetAttribLocation((ATTRIB_STD)attrib);

	// Generate warnings
	if (!m_pProgram)
		log(M3DGL_WARNING_NO_PROGRAMMABLE_PIPELINE);
	if (m_pProgram && m_attribId[ATTR_VERTEX] == (GLuint)-1)
		log(M3DGL_WARNING_VERTEX_COORDS_NOT_IMPLEMENTED);
	if (m_pProgram && m_attribId[ATTR_NORMAL] == (GLuint)-1)
		log(M3DGL_WARNING_NORMAL_COORDS_NOT_IMPLEMENTED);
	if (m_attribId[ATTR_BONE_ID] != (GLuint)-1 && m_attribId[ATTR_BONE_WEIGHT] == (GLuint)-1)
		log(M3DGL_WARNING_BONE_WEIGHTS_NOT_IMPLEMENTED);
	if (m_attribId[ATTR_BONE_ID] == (GLuint)-1 && m_attribId[ATTR_BONE_WEIGHT] != (GLuint)-1)
		log(M3DGL_WARNING_BONE_IDS_NOT_IMPLEMENTED);


	// create meshes
	m_pScene = pScene;
	m_meshes.resize(m_pScene->mNumMeshes, C3dglMesh(this));
	aiMesh** ppMesh = m_pScene->mMeshes;
	for (C3dglMesh& mesh : m_meshes)
		if (m_pProgram)
			mesh.create(*ppMesh++, m_attribId);
		else
			mesh.create(*ppMesh++);

	m_globInvT = glm::inverse(glm::make_mat4((float*)&m_pScene->mRootNode->mTransformation));	// transpose or invert - that is the question!
}

void C3dglModel::loadMaterials(const char* pTexRootPath)
{
	if (!m_pScene) return;

	m_materials.resize(m_pScene->mNumMaterials, C3dglMaterial());
	aiMaterial** ppMaterial = m_pScene->mMaterials;
	for (C3dglMaterial& material : m_materials)
		material.create(*ppMaterial++, pTexRootPath);
}

static void __createMap(const aiNode* pNode, std::map<std::string, const aiNode*>& map)
{
	map[pNode->mName.data] = pNode;
	for (aiNode* pChildNode : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		__createMap(pChildNode, map);
}

unsigned C3dglModel::loadAnimations()
{
	return loadAnimations(this);
}

unsigned C3dglModel::loadAnimations(C3dglModel* pCompatibleModel)
{
	if (pCompatibleModel == NULL)
		pCompatibleModel = this;

	const aiScene* pScene = pCompatibleModel->m_pScene;

	if (!pScene || !pScene->HasAnimations()) return 0;
	if (getBoneCount() == 0)
	{
		pCompatibleModel->log(M3DGL_WARNING_SKINNING_NOT_IMPLEMENTED);
		return 0;
	}

	// create animation look-up tables
	map<string, const aiNode*> mymap;		// map of nodes
	__createMap(GetScene()->mRootNode, mymap);


	m_animations.resize(pScene->mNumAnimations, C3dglAnimation(this));
	aiAnimation** ppAnimation = pScene->mAnimations;
	for (C3dglAnimation& animation : m_animations)
		animation.create(*ppAnimation++, mymap);
	
	return pScene->mNumAnimations;
}

void C3dglModel::destroy()
{
	if (m_pScene)
	{
		for (C3dglMesh mesh : m_meshes)
			mesh.destroy();
		for (C3dglMaterial mat : m_materials)
			mat.destroy();
		aiReleaseImport(m_pScene);
		m_pScene = NULL;
	}
}

void C3dglModel::renderNode(aiNode* pNode, glm::mat4 m)
{
	m *= glm::transpose(glm::make_mat4((GLfloat*)&pNode->mTransformation));

	// check if a shading program is active
	C3dglProgram* pProgram = C3dglProgram::GetCurrentProgram();

	// send model view matrix
	if (pProgram)
		pProgram->SendStandardUniform(UNI_MODELVIEW, m);
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&m);
	}

	for (unsigned iMesh : vector<unsigned>(pNode->mMeshes, pNode->mMeshes + pNode->mNumMeshes))
	{
		C3dglMesh* pMesh = &m_meshes[iMesh];
		C3dglMaterial* pMaterial = pMesh->getMaterial();
		if (pMaterial)
			pMaterial->render(pProgram);
		pMesh->render();
	}

	// draw all children
	for (aiNode* p : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		renderNode(p, m);
}

void C3dglModel::render(glm::mat4 matrix) 
{ 
	if (m_pScene->mRootNode) 
		renderNode(m_pScene->mRootNode, matrix);
}

void C3dglModel::render(unsigned iNode, glm::mat4 matrix)
{
	// update transform
	aiMatrix4x4 m = m_pScene->mRootNode->mTransformation;
	aiTransposeMatrix4(&m);
	matrix *= glm::make_mat4((GLfloat*)&m);

	if (iNode <= getMainNodeCount())
		renderNode(m_pScene->mRootNode->mChildren[iNode], matrix);
}

unsigned C3dglModel::getMainNodeCount() 
{ 
	return (m_pScene && m_pScene->mRootNode) ? m_pScene->mRootNode->mNumChildren : 0; 
}


void C3dglModel::getNodeTransform(aiNode* pNode, float pMatrix[16], bool bRecursive)
{
	aiMatrix4x4 m1, m2;
	if (bRecursive && pNode->mParent)
		getNodeTransform(pNode->mParent, (float*)&m1, true);

	m2 = pNode->mTransformation;
	aiTransposeMatrix4(&m2);

	*((aiMatrix4x4*)pMatrix) = m2 * m1;
}

bool C3dglModel::hasBone(std::string boneName)
{
	return m_mapBones.find(boneName) != m_mapBones.end();
}

size_t C3dglModel::getBoneId(std::string boneName)
{
	auto it = m_mapBones.find(boneName);
	if (it != m_mapBones.end())
		return it->second;
	else
		return (size_t)-1;
}

size_t C3dglModel::getOrAddBone(std::string boneName, glm::mat4 offsetMatrix)
{
	auto it = m_mapBones.find(boneName);
	if (it != m_mapBones.end())
		return it->second;
	else
	{
		size_t id = m_vecBones.size();
		m_vecBones.push_back(pair<std::string, glm::mat4>(boneName, offsetMatrix));
		m_mapBones[boneName] = id;
		return id;
	}
}

void C3dglModel::getBB(glm::vec3 BB[2]) 
{ 
	BB[0] = glm::vec3(1e10f, 1e10f, 1e10f);
	BB[1] = glm::vec3(-1e10f, -1e10f, -1e10f);
	if (m_pScene->mRootNode)
		getBB(m_pScene->mRootNode, BB); 
}

void C3dglModel::getBB(unsigned iNode, glm::vec3 BB[2])
{
	// update transform
	aiMatrix4x4 m = m_pScene->mRootNode->mTransformation;
	aiTransposeMatrix4(&m);

	BB[0] = glm::vec3(1e10f, 1e10f, 1e10f);
	BB[1] = glm::vec3(-1e10f, -1e10f, -1e10f);

	if (iNode <= getMainNodeCount())
		getBB(m_pScene->mRootNode->mChildren[iNode], BB, glm::make_mat4((GLfloat*)&m));
}

void C3dglModel::getBB(aiNode* pNode, glm::vec3 BB[2], glm::mat4 m)
{
	m *= glm::transpose(glm::make_mat4((GLfloat*)&pNode->mTransformation));

	for (unsigned iMesh : vector<unsigned>(pNode->mMeshes, pNode->mMeshes + pNode->mNumMeshes))
	{
		glm::vec3 bb[2];
		m_meshes[iMesh].getAABB(bb);

		glm::vec4 bb4[2];
		bb4[0] = m * glm::vec4(bb[0], 1);
		bb4[1] = m * glm::vec4(bb[1], 1);

		BB[0].x = min(BB[0].x, bb4[0].x);
		BB[0].y = min(BB[0].y, bb4[0].y);
		BB[0].z = min(BB[0].z, bb4[0].z);
		BB[0].x = min(BB[0].x, bb4[1].x);
		BB[0].y = min(BB[0].y, bb4[1].y);
		BB[0].z = min(BB[0].z, bb4[1].z);

		BB[1].x = max(BB[1].x, bb4[0].x);
		BB[1].y = max(BB[1].y, bb4[0].y);
		BB[1].z = max(BB[1].z, bb4[0].z);
		BB[1].x = max(BB[1].x, bb4[1].x);
		BB[1].y = max(BB[1].y, bb4[1].y);
		BB[1].z = max(BB[1].z, bb4[1].z);
	}

	// check all children
	for (aiNode* p : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		getBB(p, BB, m);
}

//////////////////////////////////////////////////////////////////////////////////////
// Articulated Animation Functions

void C3dglModel::getAnimData(unsigned iAnim, float time, vector<glm::mat4>& transforms)
{
	transforms.resize(getBoneCount());	// 16 floats per bone matrix
	if (!hasAnimation(iAnim))
	{
		if (!hasBones()) transforms.resize(1);
		aiMatrix4x4 m;
		for (unsigned iBone = 0; iBone < transforms.size(); iBone++)
			memcpy(&transforms[iBone], &m, sizeof(m));
	}
	else
	{
		float fTicksPerSecond = (float)getAnimation(iAnim)->getTicksPerSecond();
		if (fTicksPerSecond == 0) fTicksPerSecond = 25.0f;
		time = fmod(time * fTicksPerSecond, (float)getAnimation(iAnim)->getDuration());

		glm::mat4 t(1);
		m_animations[iAnim].readNodeHierarchy(time, GetScene()->mRootNode, t, transforms);
	}
}


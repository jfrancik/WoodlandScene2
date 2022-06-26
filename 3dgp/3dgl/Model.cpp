/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Model.h>
#include <3dgl/Shader.h>
#include <3dgl/Bitmap.h>

// assimp include file
#include <assimp/cimport.h>

// GLM include files
#include "../glm/vec3.hpp"
#include "../glm/vec4.hpp"
#include "../glm/mat4x4.hpp"
#include "../glm/trigonometric.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include <assert.h>

using namespace std;
using namespace _3dgl;

bool C3dglModel::load(const char* pFile, unsigned int flags)
{
	log(M3DGL_SUCCESS_IMPORTING_FILE, pFile);
	const aiScene* pScene = aiImportFile(pFile, flags);
	if (pScene == NULL)
	{
		log(M3DGL_ERROR_AI, aiGetErrorString());
		return false;
	}
	m_name = pFile;
	size_t i = m_name.find_last_of("/\\");
	if (i != string::npos) m_name = m_name.substr(i + 1);
	i = m_name.find_last_of(".");
	if (i != string::npos) m_name = m_name.substr(0, i);
	create(pScene);
	return true;
}

void __createMap(const aiNode* pNode, std::map<std::string, const aiNode*>& map)
{
	map[pNode->mName.data] = pNode;
	for (aiNode* pChildNode : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		__createMap(pChildNode, map);
}

void C3dglModel::create(const aiScene* pScene)
{
	// create meshes
	m_pScene = pScene;
	m_meshes.resize(m_pScene->mNumMeshes, C3dglMESH(this));
	aiMesh** ppMesh = m_pScene->mMeshes;
	for (C3dglMESH& mesh : m_meshes)
		mesh.create(*ppMesh++, m_maskEnabledBufData);

	m_globInvT = m_pScene->mRootNode->mTransformation;
	m_globInvT.Inverse();
}

void C3dglModel::loadMaterials(const char* pTexRootPath)
{
	if (!m_pScene) return;

	m_materials.resize(m_pScene->mNumMaterials, C3dglMAT());
	aiMaterial** ppMaterial = m_pScene->mMaterials;
	for (C3dglMAT& material : m_materials)
		material.create(*ppMaterial++, pTexRootPath);
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

	for (unsigned iAnim = 0; iAnim < pScene->mNumAnimations; iAnim++)
	{
		aiAnimation* pAnim = pScene->mAnimations[iAnim];

		m_animations.push_back(ANIMATION());
		ANIMATION& anim = m_animations[m_animations.size() - 1];
		anim.m_pAnim = pAnim;
		map<const aiNode*, std::pair<size_t, size_t> > &lookUp = anim.m_lookUp;

		// create the lookUp structure
		int i = 0;
		for (aiNodeAnim* pNodeAnim : vector<aiNodeAnim*>(pAnim->mChannels, pAnim->mChannels + pAnim->mNumChannels))
		{
			string strNodeName = pNodeAnim->mNodeName.data;
			const aiNode* pNode = mymap[strNodeName];
			size_t id = 99999;
			getBone(strNodeName, id);
			lookUp[pNode] = pair<size_t, size_t>(i++, id);
		}

		// rest of the nodes
		for (auto mypair : mymap)
		{
			// check if the aiNode is already known in the look-up table
			if (lookUp.find(mypair.second) == lookUp.end())
			{
				size_t id = 99999;
				getBone(mypair.first, id);
				lookUp[mypair.second] = pair<size_t, size_t>(0xffff, id);
			}
		}
	}
	return pScene->mNumAnimations;
}

unsigned C3dglModel::loadAnimations(const char* pFile, unsigned int flags)
{
	C3dglModel *p = new C3dglModel();
	m_auxModels.push_back(p);
	p->load(pFile, flags);
	return loadAnimations(p);
}

void C3dglModel::destroy()
{
	if (m_pScene)
	{
		for (C3dglMESH mesh : m_meshes)
			mesh.destroy();
		for (C3dglMAT mat : m_materials)
			mat.destroy();
		for (C3dglModel *p : m_auxModels)
			delete p;
		aiReleaseImport(m_pScene);
		m_pScene = NULL;
	}
}

void C3dglModel::enableBufData(ATTRIB_STD bufId, bool bEnable)
{
	if (bEnable)
		m_maskEnabledBufData |= (1 << bufId);
	else
		m_maskEnabledBufData &= ~(1 << bufId);
}

void C3dglModel::renderNode(aiNode* pNode, glm::mat4 m)
{
	aiMatrix4x4 mx = pNode->mTransformation;
	aiTransposeMatrix4(&mx);
	m *= glm::make_mat4((GLfloat*)&mx);

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
		C3dglMESH* pMesh = &m_meshes[iMesh];
		C3dglMAT* pMaterial = pMesh->getMaterial();
		if (pMaterial)
		{
			for (unsigned texUnit = GL_TEXTURE0; texUnit <= GL_TEXTURE31; texUnit++)
			{
				unsigned idTex;
				if (pMaterial->getTexture(texUnit, idTex))
				{
					glActiveTexture(texUnit);
					glBindTexture(GL_TEXTURE_2D, idTex);
				}
			}

			glm::vec3 vec;
			if (pMaterial->getAmbient(vec)) pProgram->SendStandardUniform(UNI_MAT_AMBIENT, vec);
			if (pMaterial->getDiffuse(vec)) pProgram->SendStandardUniform(UNI_MAT_DIFFUSE, vec);
			if (pMaterial->getSpecular(vec)) pProgram->SendStandardUniform(UNI_MAT_SPECULAR, vec);
			if (pMaterial->getEmissive(vec)) pProgram->SendStandardUniform(UNI_MAT_EMISSIVE, vec);
			float v;
			if (pMaterial->getShininess(v)) pProgram->SendStandardUniform(UNI_MAT_SHININESS, vec);
		}
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

	if (iNode <= getParentNodeCount())
		renderNode(m_pScene->mRootNode->mChildren[iNode], matrix);
}

//void C3dglModel::render()
//{
//	glm::mat4 m;
//	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&m);
//	render(m);
//}

//void C3dglModel::render(unsigned iNode)
//{
//	glm::mat4 m;
//	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&m);
//	render(iNode, m);
//}

void C3dglModel::getNodeTransform(aiNode* pNode, float pMatrix[16], bool bRecursive)
{
	aiMatrix4x4 m1, m2;
	if (bRecursive && pNode->mParent)
		getNodeTransform(pNode->mParent, (float*)&m1, true);

	m2 = pNode->mTransformation;
	aiTransposeMatrix4(&m2);

	*((aiMatrix4x4*)pMatrix) = m2 * m1;
}

bool C3dglModel::getBone(std::string boneName)
{
	return m_mapBones.find(boneName) != m_mapBones.end();
}

bool C3dglModel::getBone(std::string boneName, size_t& id)
{
	auto it = m_mapBones.find(boneName);
	if (it == m_mapBones.end())
		return false;
	id = it->second;
	return true;
}

bool C3dglModel::getOrAddBone(std::string boneName, size_t& id)
{
	if (getBone(boneName, id))
		return true;

	id = m_mapBones.size();
	m_mapBones[boneName] = id;
	return false;
}

bool C3dglModel::getBBNode(aiNode* pNode, aiVector3D BB[2], aiMatrix4x4* trafo)
{
	aiMatrix4x4 prev = *trafo;
	aiMultiplyMatrix4(trafo, &pNode->mTransformation);

	for (unsigned iMesh : vector<unsigned>(pNode->mMeshes, pNode->mMeshes + pNode->mNumMeshes))
	{
		aiVector3D bb[2];
		memcpy(bb, m_meshes[iMesh].getBB(), sizeof(bb));
		aiTransformVecByMatrix4(&bb[0], trafo);
		aiTransformVecByMatrix4(&bb[1], trafo);
		if (bb[0].x < BB[0].x) BB[0].x = bb[0].x;
		if (bb[0].y < BB[0].y) BB[0].y = bb[0].y;
		if (bb[0].z < BB[0].z) BB[0].z = bb[0].z;
		if (bb[1].x > BB[0].x) BB[1].x = bb[1].x;
		if (bb[1].y > BB[0].y) BB[1].y = bb[1].y;
		if (bb[1].z > BB[0].z) BB[1].z = bb[1].z;
	}

	for (aiNode* pNode : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		getBBNode(pNode, BB, trafo);

	*trafo = prev;
	return true;
}

void C3dglModel::getBB(aiVector3D BB[2])
{
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	BB[0].x = BB[0].y = BB[0].z = 1e10f;
	BB[1].x = BB[1].y = BB[1].z = -1e10f;
	getBBNode(m_pScene->mRootNode, BB, &trafo);
}

std::string C3dglModel::getName()
{
	if (m_name.empty())
		return "";
	else
		return "Model (" + m_name + ")";
}

//////////////////////////////////////////////////////////////////////////////////////
// Articulated Animation Functions

aiVector3D Interpolate(float AnimationTime, const aiVectorKey* pKeys, unsigned nKeys)
{
	// find a pair of keys to interpolate
	unsigned i = 0;
	while (i < nKeys - 1 && AnimationTime >= (float)pKeys[i + 1].mTime)
		i++;

	// if out of bounds, return the last key
	if (i >= nKeys - 1)
		return pKeys[nKeys - 1].mValue;

	// interpolate
	const aiVector3D& Start = pKeys[i].mValue;
	const aiVector3D& End = pKeys[i + 1].mValue;
	float f = (AnimationTime - (float)pKeys[i].mTime) / ((float)(pKeys[i + 1].mTime - pKeys[i].mTime));
	return Start + f * (End - Start);
}

aiQuaternion Interpolate(float AnimationTime, const aiQuatKey* pKeys, unsigned nKeys)
{
	// find a pair of keys to interpolate
	unsigned i = 0;
	while (i < nKeys - 1 && AnimationTime >= (float)pKeys[i + 1].mTime)
		i++;

	// if out of bounds, return the last key
	if (i >= nKeys - 1)
		return pKeys[nKeys - 1].mValue;

	const aiQuaternion& StartRotationQ = pKeys[i].mValue;
	const aiQuaternion& EndRotationQ = pKeys[i + 1].mValue;
	float f = (AnimationTime - (float)pKeys[i].mTime) / ((float)(pKeys[i + 1].mTime - pKeys[i].mTime));
	aiQuaternion q;
	aiQuaternion::Interpolate(q, StartRotationQ, EndRotationQ, f);	// spherical interpolation (SLERP)
	return q.Normalize();
}

void C3dglModel::readNodeHierarchy(ANIMATION& animation, float time, const aiNode* pNode, const aiMatrix4x4& t, vector<float>& transforms)
{
	aiMatrix4x4 transform;

	auto it = animation.m_lookUp.find(pNode);
	if (it != animation.m_lookUp.end())
	{
		size_t iAnimInd = it->second.first;
		if (iAnimInd < animation.m_pAnim->mNumChannels)
		{
			const aiNodeAnim* pNodeAnim = animation.m_pAnim->mChannels[iAnimInd];

			// Interpolate position, rotation and scaling
			aiVector3D vecTranslate = Interpolate(time, pNodeAnim->mPositionKeys, pNodeAnim->mNumPositionKeys);
			aiQuaternion quatRotate = Interpolate(time, pNodeAnim->mRotationKeys, pNodeAnim->mNumRotationKeys);
			aiVector3D vecScale = Interpolate(time, pNodeAnim->mScalingKeys, pNodeAnim->mNumScalingKeys);

			// create matrices
			aiMatrix4x4 matTranslate, matScale;
			aiMatrix4x4::Translation(vecTranslate, matTranslate);
			aiMatrix4x4 matRotate = aiMatrix4x4(quatRotate.GetMatrix());
			aiMatrix4x4::Scaling(vecScale, matScale);

			// Combine the above transformations
			transform = t * matTranslate * matRotate * matScale;
		}
		else
			transform = t * pNode->mTransformation;


		size_t iBone = it->second.second;
		if (iBone < m_vecBoneOffsets.size())
		{
			aiMatrix4x4 m = (m_globInvT * transform * m_vecBoneOffsets[iBone]).Transpose();
			memcpy(&transforms[iBone * 16], &m, sizeof(m));
		}
	}

	for (aiNode* pChildNode : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		readNodeHierarchy(animation, time, pChildNode, transform, transforms);
}

void C3dglModel::getAnimData(unsigned iAnim, float time, vector<float>& transforms)
{
	transforms.resize(m_vecBoneOffsets.size() * 16);	// 16 floats per bone matrix
	if (!hasAnim(iAnim))
	{
		if (m_vecBoneOffsets.size() == 0) transforms.resize(16);
		aiMatrix4x4 m;
		for (unsigned iBone = 0; iBone < transforms.size() / 16; iBone++)
			memcpy(&transforms[iBone * 16], &m, sizeof(m));
	}
	else
	{
		float fTicksPerSecond = (float)GetAnimTicksPerSecond(iAnim);
		if (fTicksPerSecond == 0) fTicksPerSecond = 25.0f;
		time = fmod(time * fTicksPerSecond, (float)GetAnimDuration(iAnim));

		aiMatrix4x4 t;
		readNodeHierarchy(m_animations[iAnim], time, GetScene()->mRootNode, t, transforms);
	}
}


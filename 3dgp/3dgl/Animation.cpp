/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <fstream>
#include <GL/glew.h>
#include <3dgl/Animation.h>
#include <3dgl/Model.h>

// assimp include file
#include <assimp/scene.h>

// GLM include files
#include "../glm/gtc/type_ptr.hpp"

using namespace std;
using namespace _3dgl;

C3dglAnimation::C3dglAnimation(C3dglModel* pOwner) : m_pOwner(pOwner), m_pAnim(NULL)
{
}

std::string C3dglAnimation::getName() 
{ 
	return m_pAnim->mName.data; 
}

double C3dglAnimation::getDuration() 
{ 
	return m_pAnim->mDuration; 
}

double C3dglAnimation::getTicksPerSecond() 
{ 
	return m_pAnim->mTicksPerSecond; 
}


void C3dglAnimation::create(const aiAnimation* pAnim, map<string, const aiNode*>& mymap)
{
	m_pAnim = pAnim;

	// create the lookUp structure
	int i = 0;
	for (aiNodeAnim* pNodeAnim : vector<aiNodeAnim*>(pAnim->mChannels, pAnim->mChannels + pAnim->mNumChannels))
	{
		string strNodeName = pNodeAnim->mNodeName.data;
		const aiNode* pNode = mymap[strNodeName];
		size_t id = m_pOwner->getBoneId(strNodeName);
		m_lookUp[pNode] = pair<size_t, size_t>(i++, id);
	}

	// rest of the nodes
	for (auto& mypair : mymap)
	{
		// check if the aiNode is already known in the look-up table
		if (m_lookUp.find(mypair.second) == m_lookUp.end())
		{
			size_t id = m_pOwner->getBoneId(mypair.first);
			m_lookUp[mypair.second] = pair<size_t, size_t>(0xffff, id);
		}
	}
}

static aiVector3D Interpolate(float AnimationTime, const aiVectorKey* pKeys, unsigned nKeys)
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

static aiQuaternion Interpolate(float AnimationTime, const aiQuatKey* pKeys, unsigned nKeys)
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

void C3dglAnimation::readNodeHierarchy(float time, const aiNode* pNode, const glm::mat4& t, std::vector<glm::mat4>& transforms)
{
	glm::mat4 transform;

	auto it = m_lookUp.find(pNode);
	if (it != m_lookUp.end())
	{
		size_t iAnimInd = it->second.first;
		if (iAnimInd < m_pAnim->mNumChannels)
		{
			const aiNodeAnim* pNodeAnim = m_pAnim->mChannels[iAnimInd];

			// Interpolate position, rotation and scaling
			aiVector3D vecTranslate = Interpolate(time, pNodeAnim->mPositionKeys, pNodeAnim->mNumPositionKeys);
			aiQuaternion quatRotate = Interpolate(time, pNodeAnim->mRotationKeys, pNodeAnim->mNumRotationKeys);
			aiVector3D vecScale = Interpolate(time, pNodeAnim->mScalingKeys, pNodeAnim->mNumScalingKeys);

			// create matrices
			aiMatrix4x4 matTranslate, matScale;
			aiMatrix4x4::Translation(vecTranslate, matTranslate);
			aiMatrix4x4 matRotate = aiMatrix4x4(quatRotate.GetMatrix());
			aiMatrix4x4::Scaling(vecScale, matScale);
			aiMatrix4x4 mat = matTranslate * matRotate * matScale;

			// Combine the above transformations
			transform = glm::make_mat4((float*)&mat) * t;
		}
		else
			transform = t * glm::make_mat4((float*)&pNode->mTransformation);


		size_t iBone = it->second.second;
		if (iBone < m_pOwner->getBoneCount())
			transforms[iBone] = glm::transpose(m_pOwner->getBone(iBone) * transform * m_pOwner->getGlobalTransposedTransform());
	}

	for (aiNode* pChildNode : vector<aiNode*>(pNode->mChildren, pNode->mChildren + pNode->mNumChildren))
		readNodeHierarchy(time, pChildNode, transform, transforms);
}

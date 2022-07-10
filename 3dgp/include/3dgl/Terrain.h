/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

A terrain class.
Usage:
load to load the height map and scale its height
render to render the terrain
getHeight or getInterpolatedHeight to obtain the height of the terrain at the given coords
----------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

   Jarek Francik
   jarek@kingston.ac.uk
*********************************************************************************/
#ifndef __3dglTerrain_h_
#define __3dglTerrain_h_

// Include GLM core features
#include "../glm/glm.hpp"

#include "Object.h"
#include "CommonDef.h"

namespace _3dgl
{
	class C3dglProgram;
	class C3dglMesh;

	class MY3DGL_API C3dglTerrain : public C3dglObject
	{
		std::string m_name;	// model name (derived from the filename)

		// shader-related data
		C3dglProgram* m_pProgram;					// program responsible for loading the model; NULL if fixed pipeline or no model loaded
		C3dglProgram* m_pLastProgramUsed;			// the last program used for rendering; NULL if never rendered since loading the model

		// height map size (may be rectangular)
		float* m_heights;			// heights
		int m_nSizeX, m_nSizeZ;		// size (may be rectangular)
		float m_fScaleHeight;		// heigth (vertical) scale

		// VAO (Vertex Array Object) id
		GLuint m_idVAO;

		// Attribute Buffer Ids
		static const size_t c_attrCount = ATTR_COLOR;	// terrains will only ever create 5 sttribute types
		GLuint m_id[c_attrCount];	// mote: only 5 attributes created by terrain objects (no colour or bone attr)
		GLuint m_idIndex;			// index buffer id

	protected:
		void createHeightMap(int nSizeX, int nSizeZ, float fScaleHeight, void* pBytes);
		size_t getBuffers(float** attrData, size_t* attrSize, size_t attrCount);
		size_t getIndexBuffer(GLuint** indexData, size_t* indSize);
		void cleanUp(float** attrData, GLuint* indexData, size_t attrCount);		// call after getBuffers well data no longer required

	public:
		C3dglTerrain();
		virtual ~C3dglTerrain() { destroy(); }

		// heightmap information
		float* getHeightMap() { return m_heights; }
		void getSize(int& nSizeX, int& nSizeZ, float& fScaleHeight) { nSizeX = m_nSizeX, nSizeZ = m_nSizeZ, fScaleHeight = m_fScaleHeight;  }
		float getHeight(int x, int z);
		float getInterpolatedHeight(float x, float z);

		bool load(const std::string filename, float scaleHeight);
		void create(int nSizeX, int nSizeZ, float fScaleHeight, void* pBytes);
		void destroy();

		void render(glm::mat4 matrix);

		std::string getName() const { return "Terrain (" + m_name + ")"; }

		friend bool MY3DGL_API convHeightmap2OBJ(const std::string fileImage, float scaleHeight, const std::string fileOBJ);
		friend bool MY3DGL_API convHeightmap2Mesh(const std::string fileImage, float scaleHeight, C3dglMesh& mesh);
	};
	
	bool MY3DGL_API convHeightmap2OBJ(const std::string fileImage, float scaleHeight, const std::string fileOBJ);
	bool MY3DGL_API convHeightmap2Mesh(const std::string fileImage, float scaleHeight, C3dglMesh& mesh);
}; // namespace _3dgl

#endif

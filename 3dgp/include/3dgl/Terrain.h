/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK

A terrain class.
Usage:
loadHeightmap to load the height map and scale its height
render to render the terrain
renderNormals to render terrain normal vectors
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

#include <string>
#include <vector>

namespace _3dgl
{
	
class MY3DGL_API C3dglTerrain
{
	// height map size (may be rectangular)
	int m_nSizeX, m_nSizeZ;
	float m_fScaleHeight;

	// buffer names
    unsigned int m_vertexBuffer;
	unsigned int m_normalBuffer;
	unsigned int m_tangentBuffer;
	unsigned int m_bitangentBuffer;
	unsigned m_texCoordBuffer;
    unsigned int m_indexBuffer;
    unsigned int m_linesBuffer;

public:
    C3dglTerrain();

	// height map
#pragma warning(push)
#pragma warning(disable: 4251)
	std::vector<float> m_heights;
#pragma warning(pop)

	float getHeight(int x, int z);
	float getInterpolatedHeight(float x, float z);

	bool loadHeightmap(const std::string filename, float scaleHeight);
	void render(glm::mat4 matrix);
	void render();
	void renderNormals();

	bool storeAsOBJ(const std::string filename);
	bool storeAsRAW(const std::string filename);
};

}; // namespace _3dgl

#endif

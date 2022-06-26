/*********************************************************************************
3DGL 3D Graphics Library created by Jarek Francik for Kingston University students
Version 3.0 - June 2022
Copyright (C) 2013-22 by Jarek Francik, Kingston University, London, UK
*********************************************************************************/
#include <iostream>
#include <GL/glew.h>
#include <3dgl/Bitmap.h>

// DevIL include file
#undef _UNICODE
#include <IL/il.h>

using namespace std;
using namespace _3dgl;

C3dglBitmap *C3dglBitmap::c_pBound = NULL;

C3dglBitmap::C3dglBitmap(std::string fname, unsigned format)
{
	m_idImage = 0;
	Load(fname, format);
}

bool C3dglBitmap::load(std::string fname, unsigned format)
{
	// initialise IL
	static bool bIlInitialised = false;
	if (!bIlInitialised)
		ilInit(); 
	bIlInitialised = true;

	// destroy previous image
	destroy();
	
	// generate IL image id
	ilGenImages(1, &m_idImage); 

	// bind IL image and load
	ilBindImage(m_idImage);
	c_pBound = this;
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT); 
	if (ilLoadImage((ILstring)fname.c_str()))
	{
		ilConvertImage(format, IL_UNSIGNED_BYTE); 
		log(M3DGL_SUCCESS_LOADED, fname);
		return true;
	}
	else 
	{
		log(M3DGL_WARNING_CANNOT_LOAD_FROM, fname);
		return false;
	}
}

void C3dglBitmap::destroy()
{
	if (m_idImage)
		ilDeleteImages(1, &m_idImage);
}

long C3dglBitmap::getWidth()
{
	if (c_pBound != this)
	{
		ilBindImage(m_idImage);
		c_pBound = this;
	}
	return ilGetInteger(IL_IMAGE_WIDTH);
}

long C3dglBitmap::getHeight()
{
	if (c_pBound != this)
	{
		ilBindImage(m_idImage);
		c_pBound = this;
	}
	return ilGetInteger(IL_IMAGE_HEIGHT);
}

void *C3dglBitmap::getBits()
{
	if (c_pBound != this)
	{
		ilBindImage(m_idImage);
		c_pBound = this;
	}
	return ilGetData();
}

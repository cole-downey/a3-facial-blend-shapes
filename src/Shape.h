#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#include <memory>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;

class Shape
{
public:
	Shape();
	virtual ~Shape();
	void loadObj(const std::string &filename, std::vector<float> &pos, std::vector<float> &nor, std::vector<float> &tex, bool loadNor = true, bool loadTex = true);
	void loadMesh(const std::string &meshName);
	void addBlendshape(const std::string &meshName);
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	virtual void init(bool bindTex = true);
	void initBlend();
	virtual void draw() const;
	virtual void blend(float t);
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	
protected:
	void calcBlendWeights(float t);
	std::string meshFilename;
	std::string textureFilename;
	std::shared_ptr<Program> prog;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<float> posBind;
	std::vector<float> norBind;
	std::vector< std::shared_ptr< std::vector<float> > > posBlendDeltas;
	std::vector< std::shared_ptr< std::vector<float> > > norBlendDeltas;
	int nBlendshapes;
	std::vector<float> blendWeights;
	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
	GLuint b1PosBufID;
	GLuint b1NorBufID;
	GLuint b2PosBufID;
	GLuint b2NorBufID;
};

#endif

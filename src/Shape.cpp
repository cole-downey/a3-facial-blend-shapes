#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Shape.h"
#include "GLSL.h"
#include "Program.h"

using namespace std;
using namespace glm;

Shape::Shape() :
	prog(NULL),
	posBufID(0),
	norBufID(0),
	texBufID(0),
	b1PosBufID(0),
	b1NorBufID(0),
	b2PosBufID(0),
	b2NorBufID(0),
	nBlendshapes(0) {
}

Shape::~Shape() {
}

void Shape::loadObj(const string& filename, vector<float>& pos, vector<float>& nor, vector<float>& tex, bool loadNor, bool loadTex) {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
	if (!warn.empty()) {
		//std::cout << warn << std::endl;
	}
	if (!err.empty()) {
		std::cerr << err << std::endl;
	}
	if (!ret) {
		return;
	}
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				pos.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				pos.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				pos.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				if (!attrib.normals.empty() && loadNor) {
					nor.push_back(attrib.normals[3 * idx.normal_index + 0]);
					nor.push_back(attrib.normals[3 * idx.normal_index + 1]);
					nor.push_back(attrib.normals[3 * idx.normal_index + 2]);
				}
				if (!attrib.texcoords.empty() && loadTex) {
					tex.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
					tex.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				}
			}
			index_offset += fv;
		}
	}
}

void Shape::loadMesh(const string& meshName) {
	// Load geometry
	meshFilename = meshName;
	loadObj(meshFilename, posBuf, norBuf, texBuf);
	loadObj(meshFilename, posBind, norBind, texBuf, true, false);
}

void Shape::addBlendshape(const string& meshName) {
	cout << "Blendshape added for: " << meshName << endl;
	vector<float> posBlend, norBlend;
	loadObj(meshName, posBlend, norBlend, texBuf, true, false);
	auto posDelta = make_shared< vector<float> >();
	auto norDelta = make_shared< vector<float> >();
	for (int i = 0; i < posBlend.size(); i++) {
		posDelta->push_back(posBlend.at(i) - posBind.at(i));
		norDelta->push_back(norBlend.at(i) - norBind.at(i));
	}
	posBlendDeltas.push_back(posDelta);
	norBlendDeltas.push_back(norDelta);
	blendWeights.push_back(0.0f);
	nBlendshapes++;
	initBlend();
}

void Shape::init(bool bindTex) {
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);

	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);

	if (bindTex) {
		// Send the texcoord array to the GPU
		glGenBuffers(1, &texBufID);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glBufferData(GL_ARRAY_BUFFER, texBuf.size() * sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	}

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void Shape::initBlend() {
	if (nBlendshapes > 0) {

		// Send the blend 1 position array to the GPU
		glGenBuffers(1, &b1PosBufID);
		glBindBuffer(GL_ARRAY_BUFFER, b1PosBufID);
		glBufferData(GL_ARRAY_BUFFER, posBlendDeltas.at(0)->size() * sizeof(float), &posBlendDeltas.at(0)->at(0), GL_STATIC_DRAW);

		// Send the blend 1 normal array to the GPU
		glGenBuffers(1, &b1NorBufID);
		glBindBuffer(GL_ARRAY_BUFFER, b1NorBufID);
		glBufferData(GL_ARRAY_BUFFER, norBlendDeltas.at(0)->size() * sizeof(float), &norBlendDeltas.at(0)->at(0), GL_STATIC_DRAW);

	}
	if (nBlendshapes > 1) {

		// Send the blend 2 position array to the GPU
		glGenBuffers(1, &b2PosBufID);
		glBindBuffer(GL_ARRAY_BUFFER, b2PosBufID);
		glBufferData(GL_ARRAY_BUFFER, posBlendDeltas.at(1)->size() * sizeof(float), &posBlendDeltas.at(1)->at(0), GL_STATIC_DRAW);

		// Send the blend 2 normal array to the GPU
		glGenBuffers(1, &b2NorBufID);
		glBindBuffer(GL_ARRAY_BUFFER, b2NorBufID);
		glBufferData(GL_ARRAY_BUFFER, norBlendDeltas.at(1)->size() * sizeof(float), &norBlendDeltas.at(1)->at(0), GL_STATIC_DRAW);
	}

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}
void Shape::draw() const {
	assert(prog);

	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// adding blend deltas
	if (nBlendshapes > 0) {
		glUniform1f(prog->getUniform("b1Weight"), blendWeights.at(0));

		int h_b1pos = prog->getAttribute("b1Pos");
		glEnableVertexAttribArray(h_b1pos);
		glBindBuffer(GL_ARRAY_BUFFER, b1PosBufID);
		glVertexAttribPointer(h_b1pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

		int h_b1nor = prog->getAttribute("b1Nor");
		glEnableVertexAttribArray(h_b1nor);
		glBindBuffer(GL_ARRAY_BUFFER, b1NorBufID);
		glVertexAttribPointer(h_b1nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

		if (nBlendshapes > 1) {
			glUniform1f(prog->getUniform("b2Weight"), blendWeights.at(1));
			int h_b2pos = prog->getAttribute("b2Pos");
			glEnableVertexAttribArray(h_b2pos);
			glBindBuffer(GL_ARRAY_BUFFER, b2PosBufID);
			glVertexAttribPointer(h_b2pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

			int h_b2nor = prog->getAttribute("b2Nor");
			glEnableVertexAttribArray(h_b2nor);
			glBindBuffer(GL_ARRAY_BUFFER, b2NorBufID);
			glVertexAttribPointer(h_b2nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
		}
	}

	// Draw
	int count = posBuf.size() / 3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);

	if (nBlendshapes > 0) {
		glDisableVertexAttribArray(prog->getAttribute("b1Pos"));
		glDisableVertexAttribArray(prog->getAttribute("b1Nor"));
		glDisableVertexAttribArray(prog->getAttribute("b2Pos"));
		glDisableVertexAttribArray(prog->getAttribute("b2Nor"));
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void Shape::blend(float t) {
	if (nBlendshapes > 0) {
		calcBlendWeights(t);
		/* CPU Blending
		for (int i = 0; i < posBuf.size(); i++) {
			float tempPos = posBind.at(i);
			float tempNor = norBind.at(i);
			for (int j = 0; j < nBlendshapes; j++) {
				tempPos += blendWeights.at(j) * posBlendDeltas.at(j)->at(i);
				tempNor += blendWeights.at(j) * norBlendDeltas.at(j)->at(i);
			}
			posBuf.at(i) = tempPos;
			norBuf.at(i) = tempNor;
		}
		for (int v = 0; v < norBuf.size(); v += 3) {
			// normalize normals
			vec3 normNor = vec3(norBuf.at(v), norBuf.at(v + 1), norBuf.at(v + 2));
			normNor = normalize(normNor);
			norBuf.at(v) = normNor.x;
			norBuf.at(v + 1) = normNor.y;
			norBuf.at(v + 2) = normNor.z;
		}
		*/
	}
}

void Shape::calcBlendWeights(float t) {
	float tSegment = 2.0f;
	float tMax = tSegment * (nBlendshapes + 1);
	t = fmodf(t, tMax);
	for (int i = 0; i < nBlendshapes; i++) {
		if (t > tSegment * i && t < tSegment * (i + 1)) {
			blendWeights.at(i) = t / tSegment - i;
		} else if (t > tSegment * (i + 1) && t < tSegment * (i + 2)) {
			blendWeights.at(i) = (i + 2) - t / tSegment;
		} else {
			blendWeights.at(i) = 0.0f;
		}
	}
}


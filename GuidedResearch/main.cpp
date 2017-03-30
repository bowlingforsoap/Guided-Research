#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include <iomanip>      // std::setprecision
#include <algorithm>

#include "labelingmanager.h"
//#include "contourer.h"
//#include "labeler.h"
//#include "renderer.h"
#include "meausrement.h"


/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint charWidth, GLint charHeight, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY);

int main() {
	GLint charWidth = 1500, charHeight = 1500;
	GLFWwindow* window = glfwInitialize(charWidth, charHeight, "Guided Research", 4, 4, false);
	glewInit();
	glViewport(0, 0, charWidth, charHeight);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const GLint fieldWidth = 1000;
	const GLint fieldHeight = 800;
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	const GLint fieldWidthPerm = fieldHeight;
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	const GLint fieldHeightPerm = fieldWidth;
	const glm::vec2 xDomain(-8.f, 8.f);
	const glm::vec2 yDomain(-3.f, 3.f);
	GLfloat* scalarField = nullptr; // TODO: don't forget to remove in the end
	generateScalarField(scalarField, fieldWidthPerm, fieldHeightPerm, xDomain.x, yDomain.x, xDomain.y, yDomain.y);

	LabelingManager labelingManager(fieldWidth, fieldHeight, xDomain, yDomain, scalarField);

	measure_start(1)
	//produceLabeledContour(5, .05f, .1f, .7f, computeProgram, contourTex, window, renderer);
	//produceLabeledContour(5, .05f, .1f, .6f, computeProgram, contourTex, window, renderer);
	//labelingManager.clearData();
	labelingManager.produceLabeledContour(5, .05f, .1f, .5f);
	labelingManager.produceLabeledContour(4, .05f, .1f, .4f);
	labelingManager.produceLabeledContour(5, .05f, .1f, .9f);
	measure_end
	labelingManager.renderContours();
	labelingManager.renderLabels();
	glfwSwapBuffers(window);

	system("pause");
	glfwTerminate();
	return 666;
}



/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY) {
	scalarField = new GLfloat[width * height];
	float xDelta = abs(maxX - minX) / width;
	float yDelta = abs(maxY - minY) / height;
	// Fill scalar field with f:sin(x)*cos(x)
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			scalarField[i * height + j] = sin(minX + xDelta * i) * cos(minY + yDelta * j);
			//assert(!isnan(scalarField[i * height + j]));
			if (isnan(scalarField[i * height + j])) { // TODO: unit test, remove
				system("pause");
			}
		}
	}
}

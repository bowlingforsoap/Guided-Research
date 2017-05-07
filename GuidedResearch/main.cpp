#include "GLFWinitializer.h"
#include "shader.h"
#include <SOIL.h>

#include <iomanip>      // std::setprecision
#include <algorithm>

#include "labelingmanager.h"
#include "meausrement.h"


/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint charWidth, GLint charHeight, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY);

// Debug, remove
GLFWwindow* window;

int main() {
	// Window and GL setup
	GLint windowWidth = 1500, windowHeight = 1500;
	window = glfwInitialize(windowWidth, windowHeight, "Guided Research", 4, 4, false);
	glewInit();
	glViewport(0, 0, windowWidth, windowHeight);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Scalar field setup and generation
	GLint fieldWidth = 100;
	GLint fieldHeight = 40; 
	/*GLint fieldWidth = 1000;
	GLint fieldHeight = 800;*/
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	GLint fieldWidthPerm = fieldHeight;
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	GLint fieldHeightPerm = fieldWidth;
	glm::vec2 xDomain(-8.f, 8.f);
	glm::vec2 yDomain(-3.f, 3.f);
	GLfloat* scalarField = nullptr; // TODO: don't forget to remove in the end
	generateScalarField(scalarField, fieldWidthPerm, fieldHeightPerm, xDomain.x, yDomain.x, xDomain.y, yDomain.y);

	LabelingManager labelingManager(fieldWidth, fieldHeight, scalarField);

	// Performance test
	measure_start(50)
	glClear(GL_COLOR_BUFFER_BIT);
	labelingManager.clearData();
	labelingManager.produceLabeledContour(5, .05f, .1f, .5f);
	//labelingManager.renderContours();
	//labelingManager.renderLabels();
	//glfwSwapBuffers(window);
	//// Debug
	//glClear(GL_COLOR_BUFFER_BIT);

	//labelingManager.renderContours();
	//labelingManager.renderLabels();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//for (Labeler::Label label : labelingManager.addedLabels) {
	//	const Point aabbPoints[]{
	//		Point{ label.aabb.left, label.aabb.bottom },
	//		Point{ label.aabb.left, label.aabb.top },
	//		Point{ label.aabb.right, label.aabb.bottom },
	//		Point{ label.aabb.right, label.aabb.top }
	//	};
	//	labelingManager.renderer.render2Dsmth(aabbPoints, 4, GL_TRIANGLE_STRIP, true);
	//}
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glfwSwapBuffers(window);
	//glfwSwapBuffers(window);
	////
	//glfwSwapBuffers(window);


	labelingManager.produceLabeledContour(4, .05f, .1f, .4f);
	/*labelingManager.renderContours();
	labelingManager.renderLabels();
	glfwSwapBuffers(window);*/
	// Debug
	//glClear(GL_COLOR_BUFFER_BIT);

	//labelingManager.renderContours();
	//labelingManager.renderLabels();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//for (Labeler::Label label : labelingManager.addedLabels) {
	//	Point aabbPoints[]{
	//		Point{ label.aabb.left, label.aabb.bottom },
	//		Point{ label.aabb.left, label.aabb.top },
	//		Point{ label.aabb.right, label.aabb.bottom },
	//		Point{ label.aabb.right, label.aabb.top }
	//	};
	//	labelingManager.renderer.render2Dsmth(aabbPoints, 4, GL_TRIANGLE_STRIP, true);
	//	glfwSwapBuffers(window);
	//	glfwSwapBuffers(window);
	//}
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glfwSwapBuffers(window);
	//glfwSwapBuffers(window);
	////
	//glfwSwapBuffers(window);

	labelingManager.produceLabeledContour(5, .05f, .1f, .9f);
	labelingManager.renderContours();
	labelingManager.renderLabels();
	glfwSwapBuffers(window);
	// Debug
	/*glClear(GL_COLOR_BUFFER_BIT);

	labelingManager.renderContours();
	labelingManager.renderLabels();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (Labeler::Label label : labelingManager.addedLabels) {
		const Point aabbPoints[]{
			Point{ label.aabb.left, label.aabb.bottom },
			Point{ label.aabb.left, label.aabb.top },
			Point{ label.aabb.right, label.aabb.bottom },
			Point{ label.aabb.right, label.aabb.top }
		};
		labelingManager.renderer.render2Dsmth(aabbPoints, 4, GL_TRIANGLE_STRIP, true);
		glfwSwapBuffers(window);
		glfwSwapBuffers(window);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glfwSwapBuffers(window);
	glfwSwapBuffers(window);*/
	//
	// Optional test on dynamic scalar field size
	/*const glm::vec2 xDomain(-5.f, 8.f);
	const glm::vec2 yDomain(-3.f, 3.f);
	fieldWidth = 100;
	fieldHeight = 100;
	delete[] scalarField;
	generateScalarField(scalarField, fieldHeight, fieldWidth, xDomain.x, yDomain.x, xDomain.y, yDomain.y);
	labelingManager.updateScalarFieldSize(scalarField, fieldWidth, fieldHeight);
	labelingManager.produceLabeledContour(5, .05f, .1f, .3f);
	labelingManager.produceLabeledContour(4, .05f, .1f, .7f);
	labelingManager.produceLabeledContour(5, .05f, .1f, .9f);*/

	measure_end
	
	/*labelingManager.renderContours();
	labelingManager.renderLabels();
	glfwSwapBuffers(window);*/

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

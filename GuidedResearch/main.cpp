#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include "contourer.h"

/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize);

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length);

// Renders Transform Feedback results.
inline void testTransformFeedback(int numPrimitives, GLFWwindow& window, GLuint& prevTFBO);

int main() {
	GLint width = 1600, height = 1000;
	GLFWwindow* window = glfwInitialize(width, height, "Guided Research", 4, 2, false);
	glewInit();
	glViewport(0, 0, width, height);

	// Setup shaders
	Shader shader("vert.glsl", "frag.glsl", "geometry.glsl");
	// Check some OpenGL capabilities
	GLint value;
	glGetIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &value);
	cout << "GL_MAX_GEOMETRY_IMAGE_UNIFORMS: " << value << endl;
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &value);
	cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << value << endl;

	// Scalar Field setup
	GLint fieldWidth = 100, fieldHeight = 100;
	GLfloat* scalarField = 0;
	GLint* fieldCoords = 0;
	GLint fieldCoordsSize;
	generateScalarField(scalarField, fieldWidth, fieldHeight, -3, -3, 3, 3, fieldCoords, fieldCoordsSize);

	// Check scalar field coords
	/*cout << "\nScalar field:" << endl;
	for (int i = 0; i < 20; i++) {
		cout << scalarField[i] << ", ";
	}
	cout << endl;
	cout << "\nScalar field end:" << endl;
	for (int i = fieldWidth * fieldHeight - 1; i > (fieldWidth * fieldHeight - 1) - 20; i--) {
		cout << scalarField[i] << ", ";
	}
	cout << endl << "\nField coords:" << endl;
	for (int i = 0; i < 20; i++) {
		cout << fieldCoords[i] << ", ";
	}
	cout << endl;
	cout << "\nField coords end:" << endl;
	for (int i = fieldCoordsSize - 1; i > fieldCoordsSize - 20; i--) {
		cout << fieldCoords[i] << ", ";
	}*/

	// Set uniforms
	shader.Use();
	glUniform1i(glGetUniformLocation(shader.Program, "scalarField"), 0); // image unit 0
	glUniform3f(glGetUniformLocation(shader.Program, "isoValue"), .5f, .4f, .8f);

	// Store scalar field into a texture
	GLuint scalarFieldTex;
	glGenTextures(1, &scalarFieldTex);
	glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
	glBindImageTexture(0, scalarFieldTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindTexture(GL_TEXTURE_2D, 0);
	//cout << glGetError() << endl;
	delete[] scalarField;

	// Setup attributes
	GLuint VAO, VBO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	// Bind data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, fieldCoordsSize * sizeof(GLint), fieldCoords, GL_STATIC_DRAW);
	// Setup vertex array
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_INT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	// Transform Feedback setup
	GLuint TFBO;
	glGenBuffers(1, &TFBO);
	glBindBuffer(GL_ARRAY_BUFFER, TFBO);
	// Theoretically each square can have up to 4 points, each point has 2 coords in GLFfloat -> fieldHeight * fieldWidth * 4 * 2 * sizeof(GLfloat)
	glBufferData(GL_ARRAY_BUFFER, fieldHeight * fieldWidth * 8 * sizeof(GLfloat), nullptr, GL_STATIC_READ);
	// Bind TFBO to 0th binding point
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, TFBO);
	// Specify feedback varyings
	const GLchar* feedbackVaryings[]{ "gs_Feedback" };
	glTransformFeedbackVaryings(shader.Program, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	// Relink program for changes to be commited
	glLinkProgram(shader.Program);
	cout << "Error check: " << glGetError() << endl;

	// Line width
	glLineWidth(5.f);

	// Create a query to later query OpenGL
	GLuint query;
	glGenQueries(1, &query);

	glClearColor(0.1f, 0.2, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	shader.Use();
	// Query OpenGL about how many primitives were transform feedbacked
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
	glBeginTransformFeedback(GL_LINES);
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, fieldCoordsSize / 2);
	// TODO: potential needs to be placed after glEndTransformFeedback
	glBindVertexArray(0);
	glEndTransformFeedback();
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	glfwSwapBuffers(window);

	// Get query result: number of primitives written
	GLint numPrimitives;
	glGetQueryObjectiv(query, GL_QUERY_RESULT, &numPrimitives);
	cout << "Number of primitives written: " << numPrimitives << endl;

	// Read data from the GL_TRANSFORM_FEEDBACK_BUFFER
	cout << "GL_TRANSFORM_FEEDBACK_BUFFER main. ";
	printBufferContents(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(GLfloat) * 50, sizeof(GLfloat) * 4);

	// Test what we got from TF
	//testTransformFeedback(numPrimitives, *window, TFBO);

	// Sort the primitives and retrieve the contour
	vector<vector<Point>> contour = getContour(glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY), numPrimitives);
	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

	cout << "contour size: " << contour.size() << endl;
		
	system("pause");
	// Once a transform feedback object is no longer needed, it should be deleted.
	// TFBO is not a TF buffer, but a GL_ARRAY_BUFFER!
	//glDeleteTransformFeedbacks(1, &TFBO);
	glfwTerminate();
	return 666;
}

/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize) {
	scalarField = new GLfloat[width * height];
	float xDelta = abs(maxX - minX) / width;
	float yDelta = abs(maxY - minY) / height;
	// Fill scalar field with f:sin(x)*cos(x)
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			scalarField[i * height + j] = sin(minX + xDelta * i) * cos(minY + yDelta * j);
			//assert(!isnan(scalarField[i * height + j]));
			if (isnan(scalarField[i * height + j])) {
				system("pause");
			}
		}
	}

	// Fill fieldCoords: coordinates of the upper left corner of each square in context of marching squares
	fieldCoordsSize = (width - 1) * (height - 1) * 2;
	fieldCoords = new GLint[fieldCoordsSize];
	int i = 0;
	while (i < (width - 1) * (height - 1)) {
		fieldCoords[i * 2] = i / (height - 1);
		fieldCoords[i * 2 + 1] = i % (height - 1);
		i = i++;
	}
}

// Renders Transform Feedback results.
void testTransformFeedback(int numPrimitives, GLFWwindow& window, GLuint& prevTFBO) {
	//===========================================================
	// Simple test for Transform Feedback data
	cout << "Error check: " << glGetError() << endl;
	Shader testShader("simpleVert.glsl", "simpleFrag.glsl", "");

	// Fill VBO from TFBO
	GLint numBytesInFeedback = numPrimitives * 2 * 2 * sizeof(GLfloat); // numPrimitives * verticesPerPrimitive * coordsPerVertex * sizeof(GLfloat)
	GLuint testVBO;
	glGenBuffers(1, &testVBO);
	glBindBuffer(GL_ARRAY_BUFFER, testVBO);
	glBufferData(GL_ARRAY_BUFFER, numBytesInFeedback, nullptr, GL_STATIC_DRAW);
	glCopyBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_ARRAY_BUFFER, 0, 0, numBytesInFeedback);

	// Delete buffer base for transform feedback
	glDeleteBuffers(1, &prevTFBO);

	// Сheck of what's in buffer
	cout << "GL_ARRAY_BUFFER test fill. ";
	printBufferContents(GL_ARRAY_BUFFER, sizeof(GLfloat) * 50, sizeof(GLfloat) * 4);

	// Setup vertex arrays
	GLuint testVAO;
	glGenVertexArrays(1, &testVAO);
	glBindVertexArray(testVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);

	// Setup Transform Feedback for testing
	GLuint TFBO;
	glGenBuffers(1, &TFBO);
	glBindBuffer(GL_ARRAY_BUFFER, TFBO);
	glBufferData(GL_ARRAY_BUFFER, numBytesInFeedback * 2, nullptr, GL_STATIC_READ);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, TFBO);
	// Set Feedback Varying
	GLchar* feedbackVaryings[]{ "vs_Feedback" };
	glTransformFeedbackVaryings(testShader.Program, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(testShader.Program);

	//glClearColor(1.f, 0.5f, 0.1f, 1.f);
	//glClear(GL_COLOR_BUFFER_BIT);
	// Set point size big for convenience purposes
	glPointSize(20.f);

	testShader.Use();
	glBeginTransformFeedback(GL_POINTS);
	glBindVertexArray(testVBO);
	glDrawArrays(GL_POINTS, 0, numPrimitives * 2);
	glBindVertexArray(0);
	glEndTransformFeedback();

	// Read data from the GL_TRANSFORM_FEEDBACK_BUFFER
	cout << "GL_TRANSFORM_FEEDBACK_BUFFER test. ";
	printBufferContents(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(GLfloat) * 50, sizeof(GLfloat) * 4);

	glfwSwapBuffers(&window);
}

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length) {
	GLfloat* feedback = (GLfloat*)glMapBufferRange(buffer, offset, length, GL_MAP_READ_BIT);
	int feedbackSize = length / sizeof(GLfloat);

	cout << "Buffer contents: ";
	for (int i = 0; i < feedbackSize; i++) {
		cout << feedback[i] << ", ";
	}
	cout << endl;

	glUnmapBuffer(buffer);
}
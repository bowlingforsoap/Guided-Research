#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include <iomanip>      // std::setprecision

#include "contourer.h"

/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize);

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length);

int main() {
	GLint width = 1600, height = 1000;
	GLFWwindow* window = glfwInitialize(width, height, "Guided Research", 4, 4, false);
	glewInit();
	glViewport(0, 0, width, height);

	// Setup compute shader
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	string computeSrcString = Shader::getShaderCode("shaders/marchingsquares/compute.glsl");
	const GLchar* computeSrc = computeSrcString.c_str();
	glShaderSource(computeShader, 1, &computeSrc, NULL);
	glCompileShader(computeShader);
	Shader::checkShaderCompilationStatus(computeShader, computeSrc);
	// Setup compute program 
	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);
	Shader::checkProgramLinkingStatus(computeProgram);

	glDeleteShader(computeShader);

	// Check some OpenGL capabilities
	GLint value;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
	cout << "MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << value << endl;
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &value);
	cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << value << endl;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &value);
	cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << value << endl;

	// Scalar Field setup
	const GLint fieldWidth = 100, fieldHeight = 100;
	GLfloat* scalarField = nullptr;
	GLint* fieldCoords = nullptr;
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

	

	// Store scalar field into a texture
	glActiveTexture(GL_TEXTURE0);
	GLuint scalarFieldTex;
	glGenTextures(1, &scalarFieldTex);
	glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
	glBindImageTexture(0, scalarFieldTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindTexture(GL_TEXTURE_2D, 0);
	//cout << glGetError() << endl;
	delete[] scalarField;

	// Setup the output image for the texture
	glActiveTexture(GL_TEXTURE1);
	GLuint contourTex;
	const GLfloat dummyValue = 666.f;
	glGenTextures(1, &contourTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG32F, fieldWidth, fieldHeight, 4);
	glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyValue);
	GL_ERROR_CHECK
	// No need to use, to fill with empty ddata. When used with nullptr as a data, causes access violation.
	//glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, fieldWidth, fieldHeight, 4, GL_RED, GL_FLOAT, nullptr);
	//glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, fieldWidth, fieldHeight, 4, GL_RED, GL_FLOAT, nullptr);
	//glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 2, fieldWidth, fieldHeight, 4, GL_RED, GL_FLOAT, nullptr);
	//glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 3, fieldWidth, fieldHeight, 4, GL_RED, GL_FLOAT, nullptr);
	glBindImageTexture(1, contourTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	// Set uniforms
	glUseProgram(computeProgram);
	glUniform3f(glGetUniformLocation(computeProgram, "isoValue"), .5f, .4f, .8f);
	// TODO: check for zeroes
	glUniform2f(glGetUniformLocation(computeProgram, "domainUpperBound"), fieldWidth - 1, fieldHeight - 1);
	// Image units uniforms
	glUniform1i(glGetUniformLocation(computeProgram, "scalarField"), 0);
	glUniform1i(glGetUniformLocation(computeProgram, "contour"), 1);

	glClearColor(0.1f, 0.2, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDispatchCompute(1, 100, 1);
	// Ensure that writes by the compute shader have completed 
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glfwSwapBuffers(window);

	// Read back the 2D array texture
	const int contourTexSize = fieldWidth * fieldHeight * 2 * 4; // Texture size * components per element * layers in array texture
	GLfloat contourTexData[fieldWidth * 4][fieldHeight * 2];
	cout << "sizeof(contourTexData): " << sizeof(contourTexData) << endl;
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RG, GL_FLOAT, contourTexData);
//	cout << "contourTexData[contourTexSize - 1]: " << contourTexData[contourTexSize - 1] << endl;
	cout << "contourTexData: ";
	for (int i = 1200; i < 1220; i++) {
		//if (contourTexData[i] != 0)
			cout << setprecision(100) << "[i:" << i << ",  "<< contourTexData[i / 200][i % 200] << "] ";
	}
	cout << "." << endl;

	// Sort the primitives and retrieve the contour
	vector<vector<Point>> contour = getContour(contourTexData);
	cout << "contour size: " << contour.size() << endl;

	// Draw the contour
	//renderContour(*window, contour);
		
	system("pause");
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

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length) {
	GLfloat* feedback = (GLfloat*)glMapBufferRange(buffer, offset, length, GL_MAP_READ_BIT);
	int feedbackSize = length / sizeof(GLfloat);

	cout << "Buffer contents: ";
	for (int i = 0; i < feedbackSize; i++) {
		cout << /*setprecision(20) <<*/ feedback[i] << ", ";
	}
	cout << endl;

	glUnmapBuffer(buffer);
}
#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include <iomanip>      // std::setprecision

#include "contourer.h"
#include "labeler.h"
#include "renderer.h"


/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, charWidth - 2], [0, charHeight - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint charWidth, GLint charHeight, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize);

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length);

vector<Labeler::Label> Labeler::overlappingLabels; // Debug
GLFWwindow* Labeler::window;

template<size_t cols, size_t rows>
void printImageContents(const GLuint& contourTex, GLfloat(&contourTexData)[cols][rows], const GLfloat& dummyValue) {
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RG, GL_FLOAT, contourTexData);
	cout << "contourTexData: \n";
	for (int i = 0; i < rows; ++i)
	{
		/*if (i % 5 == 0)
		{
			cout << '\n';
		}*/

		for (int j = 0; j < cols; j = j + 2)
		{
			{
				if (contourTexData[i][j] != dummyValue && contourTexData[i][j + 1] != dummyValue && contourTexData[i][j] > 1.f && contourTexData[i][j] < -1.f && contourTexData[i][j + 1] > 1.f && contourTexData[i][j + 1] < -1.f)
					cout << "(" << contourTexData[i][j] << ", " << contourTexData[i][j + 1] << "), ";
			}
		}
		//cout << '\n';
	}
	//cout << "." << endl;
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

int main() {
	GLint charWidth = 1500, charHeight = 1500;
	GLFWwindow* window = glfwInitialize(charWidth, charHeight, "Guided Research", 4, 4, false);
	Labeler::window = window;// debug
	glewInit();
	glViewport(0, 0, charWidth, charHeight);

	// Setup compute shader
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	string computeSrcString = Shader::getShaderCode("shaders/marchingsquares/marchingsquares.comp");
	const GLchar* computeSrc = computeSrcString.c_str();
	glShaderSource(computeShader, 1, &computeSrc, NULL);
	glCompileShader(computeShader);
	Shader::checkShaderCompilationStatus(computeShader, computeSrc);
	// Setup compute program
	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);
	Shader::checkProgramLinkingStatus(computeProgram);
	// Delete shader
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
	// TODO: Issues when fieldWidth != fieldHeight. Maybe the same ol' problem, but contouring must have something to do with it too.
	const GLint fieldWidth = 10;
	const GLint fieldHeight = 10;
	GLfloat* scalarField = nullptr;
	GLint* fieldCoords = nullptr;
	GLint fieldCoordsSize;
	generateScalarField(scalarField, fieldWidth, fieldHeight, -8, -8, 8, 8, fieldCoords, fieldCoordsSize);
	//generateScalarField(scalarField, fieldWidth, fieldHeight, -8, -3, 8, 3, fieldCoords, fieldCoordsSize);

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
	GL_ERROR_CHECK;
	delete[] scalarField;

	// Setup the output image for the texture
	glActiveTexture(GL_TEXTURE1);
	GLuint contourTex;
	const GLfloat dummyValue = 666.f;
	GLfloat dummyArray[2]{dummyValue, dummyValue};
	glGenTextures(1, &contourTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG32F, fieldWidth, fieldHeight, 4);
	glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyArray[0]);
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

//	glClearColor(0.1f, 0.2, 0.1f, 1.f);
//	glClear(GL_COLOR_BUFFER_BIT);

	// COMPUTER SHADER
	glDispatchCompute(fieldWidth - 1, fieldHeight - 1, 1);
	// Ensure that writes by the compute shader have completed
	//glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//glfwSwapBuffers(window);

	// Read back the 2D array texture
	GLfloat contourTexData[fieldWidth * 4][fieldHeight * 2];
	cout << "sizeof(contourTexData): " << sizeof(contourTexData) << endl;
	printImageContents(contourTex, contourTexData, dummyValue);

	// Sort the primitives and retrieve the contour
	vector<vector<Point>> contour = getContour(contourTexData, fieldWidth, fieldHeight, dummyValue);
	cout << "contour size: " << contour.size() << endl;

	// Find candidate positions for the contour
	vector<vector<GLfloat>> angles = Labeler::computeCurvatureAngles(contour);
	vector<vector<Point>> candidatePositions;
	candidatePositions.reserve(contour.size());
	// Construct labels
	Labeler::Label label(10, .025f, .1f);
	vector<Labeler::Label> labels(contour.size(), label);
	// Look for candidate positions
	// Turn on wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	glfwSwapBuffers(window);
	for (int i = 0; i < contour.size(); i++) {
		// Find candidate position
		Labeler::CandidatePosition candidatePosition = Labeler::findCandidatePositions(label.getTotalWidth(), label.charHeight, contour[i], angles[i]);
		candidatePositions.push_back(candidatePosition.position);

		// Place label onto it
		labels[i].straight = candidatePosition.straight;
		Labeler::positionLabelOnLine(labels[i], candidatePositions[i]);

		// Render label
		glfwSwapBuffers(window);
		renderLabel(Labeler::labelToPositionsArray(labels[i]));
		glfwSwapBuffers(window);

		// Check for intersections
		Labeler::intersect(labels, i, labels[i]);
	}
	// Turn off wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Debug
	// Render the overlapping contours
	glfwSwapBuffers(window);
	for (Labeler::Label label : Labeler::overlappingLabels) {
		renderLabel(Labeler::labelToPositionsArray(label));
	}
	glfwSwapBuffers(window);

	glfwSwapBuffers(window);
	// Draw the contour
	srand(glfwGetTime());
	renderContour(false, contour, GL_LINE_STRIP);
	/*glfwSwapBuffers(window);
	glfwSwapBuffers(window);*/

	// Render labels
	/*glm::mat4 mvp(1.f);
	mvp = glm::translate(mvp, glm::vec3(.5f, 0.f, 0.f));
	mvp = glm::rotate(mvp, static_cast<GLfloat>(15. * M_PI / 180.), glm::vec3(0.f, 0.f, 1.f));
	Labeler::LabelCharacter labelChar(.05f, .1f);
	for (int i = 0; i < 4; i++) {
		glm::vec4 transformedPoint = mvp* glm::vec4(labelChar.points[i].x, labelChar.points[i].y, 0.f, 1.f);
		labelChar.points[i] = Point{transformedPoint.x, transformedPoint.y};
	}*/

	// May crash because of the case described in findCandidatePositions TODO
	// Draw candidate positions
	renderContour(true, candidatePositions, GL_LINE_STRIP);
	glfwSwapBuffers(window);
	//glfwSwapBuffers(window);

	system("pause");
	glfwTerminate();
	return 666;
}

/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, charWidth - 2], [0, charHeight - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
void generateScalarField(GLfloat* &scalarField, GLint charWidth, GLint charHeight, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize) {
	scalarField = new GLfloat[charWidth * charHeight];
	float xDelta = abs(maxX - minX) / charWidth;
	float yDelta = abs(maxY - minY) / charHeight;
	// Fill scalar field with f:sin(x)*cos(x)
	for (int i = 0; i < charWidth; i++) {
		for (int j = 0; j < charHeight; j++) {
			scalarField[i * charHeight + j] = sin(minX + xDelta * i) * cos(minY + yDelta * j);
			//assert(!isnan(scalarField[i * charHeight + j]));
			if (isnan(scalarField[i * charHeight + j])) {
				system("pause");
			}
		}
	}

	// Fill fieldCoords: coordinates of the upper left corner of each square in context of marching squares
	fieldCoordsSize = (charWidth - 1) * (charHeight - 1) * 2;
	fieldCoords = new GLint[fieldCoordsSize];
	int i = 0;
	while (i < (charWidth - 1) * (charHeight - 1)) {
		fieldCoords[i * 2] = i / (charHeight - 1);
		fieldCoords[i * 2 + 1] = i % (charHeight - 1);
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

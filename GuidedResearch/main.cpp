#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include <iomanip>      // std::setprecision
#include <algorithm>

#include "contourer.h"
#include "labeler.h"
#include "renderer.h"
#include "main.h"


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

const GLint fieldWidth = 10;
const GLint fieldHeight = 10;
vector<Labeler::Label> addedLabels;

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
	GLfloat* scalarField = nullptr;
	GLint* fieldCoords = nullptr;
	GLint fieldCoordsSize;
	generateScalarField(scalarField, fieldWidth, fieldHeight, -3, -8, 3, 8, fieldCoords, fieldCoordsSize);
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


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	produceLabeledContour(scalarField, .5f, computeProgram, window);
	produceLabeledContour(scalarField, .4f, computeProgram, window);
	glfwSwapBuffers(window);
	glfwSwapBuffers(window);

	for (Labeler::Label label : addedLabels) {
		renderLabel(Labeler::labelToPositionsArray(label));
	}
	glfwSwapBuffers(window);

	system("pause");
	glfwTerminate();
	return 666;
}

void produceLabeledContour(GLfloat * scalarField, GLfloat isoValue, const GLuint &computeProgram, GLFWwindow * window)
{
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
	//delete[] scalarField;

	// Setup the output image for the texture
	glActiveTexture(GL_TEXTURE1);
	GLuint contourTex;
	GLfloat dummyArray[2]{ dummyValue, dummyValue };
	glGenTextures(1, &contourTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG32F, fieldWidth, fieldHeight, 4);
	glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyArray[0]);
	glBindImageTexture(1, contourTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	// Set uniforms
	glUseProgram(computeProgram);
	glUniform1f(glGetUniformLocation(computeProgram, "isoValue"), isoValue);
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

	// Read back the 2D array texture
	GLfloat contourTexData[fieldWidth * 4][fieldHeight * 2];
	cout << "sizeof(contourTexData): " << sizeof(contourTexData) << endl;
	printImageContents(contourTex, contourTexData, dummyValue); // actually retrieves the image from OpenGL ¯\_(ツ)_/¯

	// Sort the primitives and retrieve the contour
	vector<vector<Point>> contour = getContour(contourTexData, fieldWidth, fieldHeight);
	cout << "contour size: " << contour.size() << endl;

	// Find candidate positions for the contour
	vector<vector<GLfloat>> angles = Labeler::computeCurvatureAngles(contour);
	vector<vector<Point>> finalCandidatePositions;
	finalCandidatePositions.reserve(contour.size());
	// Construct labels
	const Labeler::Label label(10, .05f, .1f);
	vector<Labeler::Label> labels(contour.size(), label);
	// Look for candidate positions
	// Turn on wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (int i = 0; i < contour.size(); i++) {
		// Find candidate positions
		vector<Labeler::CandidatePosition> candidatePositions = Labeler::findCandidatePositions(label.getTotalWidth(), label.charHeight, contour[i], angles[i]);
		// Sort by ascending curvature
		std::sort(candidatePositions.begin(), candidatePositions.end(), [](const Labeler::CandidatePosition& a, const Labeler::CandidatePosition& b) {return a.curvature < b.curvature; });

		bool candidatePositionWasFound = false;
		Labeler::Label positionedLabel;
		for (const Labeler::CandidatePosition& candidatePosition : candidatePositions) {
			// Place label onto candidatePosition
			positionedLabel = label;
			positionedLabel.straight = candidatePosition.straight;
			Labeler::positionLabelOnLine(positionedLabel, candidatePosition.position);

			// Check for intersections
			if (!Labeler::intersect(addedLabels, positionedLabel)) {
				candidatePositionWasFound = true;

				// ~Debug
				finalCandidatePositions.push_back(candidatePosition.position);
				/*render2Dsmth(&candidatePosition.position[0], candidatePosition.position.size(), GL_LINE_STRIP, glm::vec3(1.f, 0.f, 0.f));
				glfwSwapBuffers(window);
				glfwSwapBuffers(window);*/
				
				addedLabels.push_back(positionedLabel);

				break;
			}
			else {
				continue;
			}
		}

		// If all intersect with something
		if (!candidatePositionWasFound) {
			positionedLabel = label;
			Labeler::positionLabelOnLine(positionedLabel, candidatePositions[0].position);
			finalCandidatePositions.push_back(candidatePositions[0].position);
			addedLabels.push_back(positionedLabel);
		}
	}

	// Turn off wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Debug
	// Render the overlapping contours
	/*for (Labeler::Label label : Labeler::overlappingLabels) {
	renderLabel(Labeler::labelToPositionsArray(label));
	}*/

	// Draw the contour
	srand(glfwGetTime());
	renderContour(false, contour, GL_LINE_STRIP);

	// May crash because of the case described in findCandidatePositions TODO
	// Draw candidate positions
	renderContour(true, finalCandidatePositions, GL_LINE_STRIP);
	//glfwSwapBuffers(window);
	//glfwSwapBuffers(window);
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

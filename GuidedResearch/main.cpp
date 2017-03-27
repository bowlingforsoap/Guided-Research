#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

#include <iomanip>      // std::setprecision
#include <algorithm>

#include "contourer.h"
#include "labeler.h"
#include "renderer.h"
#include "meausrement.h"


/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
// Generates scalar field (which can be placed in a texture) and texture coordinates for it.
inline void generateScalarField(GLfloat* &scalarField, GLint charWidth, GLint charHeight, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize);

// Print contents of currently bound GL_TRANSFORM_FEEDBACK_BUFFER as GLfloat.
inline void printBufferContents(int buffer, GLintptr offset, GLsizei length);

vector<Labeler::Label> Labeler::overlappingLabels; // Debug
GLFWwindow* Labeler::window;

template<size_t cols, size_t rows>
void readImageContents(const GLuint& contourTex, GLfloat(&contourTexData)[cols][rows], const GLfloat& dummyValue) {
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RG, GL_FLOAT, contourTexData);
	//cout << "contourTexData: \n";
	//for (int i = 0; i < rows; ++i)
	//{
	//	/*if (i % 5 == 0)
	//	{
	//		cout << '\n';
	//	}*/

	//	for (int j = 0; j < cols; j = j + 2)
	//	{
	//		{
	//			if (contourTexData[i][j] != dummyValue && contourTexData[i][j + 1] != dummyValue && contourTexData[i][j] > 1.f && contourTexData[i][j] < -1.f && contourTexData[i][j + 1] > 1.f && contourTexData[i][j + 1] < -1.f)
	//				cout << "(" << contourTexData[i][j] << ", " << contourTexData[i][j + 1] << "), ";
	//		}
	//	}
	//	//cout << '\n';
	//}
	//cout << "." << endl;
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

const GLint fieldWidth = 200;
const GLint fieldHeight = 200;
// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
const GLint fieldWidthPerm = fieldHeight;
// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
const GLint fieldHeightPerm = fieldWidth;
// Left and right domain boundaries
//const glm::vec2 xDomain(-.5f, 3.5f);
//const glm::vec2 yDomain(-2.f, 2.f);
const glm::vec2 xDomain(-8.f, 8.f);
const glm::vec2 yDomain(-3.f, 3.f);
vector<Labeler::Label> addedLabels;

void produceLabeledContour(const int& numChars, const GLfloat& charWidth, const GLfloat& charHeight, GLfloat isoValue, const GLuint &computeProgram, const GLuint& contourTex, GLFWwindow * window, Renderer& renderer)
{
	GLfloat dummyArray[2]{ dummyValue, dummyValue };
	glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyArray[0]);

	glUseProgram(computeProgram);
	glUniform1f(glGetUniformLocation(computeProgram, "isoValue"), isoValue);
	// TODO: check for zeroes
	glUniform2f(glGetUniformLocation(computeProgram, "domainUpperBound"), static_cast<GLfloat>(fieldWidth - 1), static_cast<GLfloat>(fieldHeight - 1));

	//	glClearColor(0.1f, 0.2, 0.1f, 1.f);
	//	glClear(GL_COLOR_BUFFER_BIT);

	// COMPUTER SHADER
	glDispatchCompute(fieldWidth - 1, fieldHeight - 1, 1);
	// Ensure that writes by the compute shader have completed
	//glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Read back the 2D array texture
	GLfloat contourTexData[fieldWidthPerm * 4][fieldHeightPerm * 2];
	//cout << "sizeof(contourTexData): " << sizeof(contourTexData) << endl;
	readImageContents(contourTex, contourTexData, dummyValue); // actually retrieves the image from OpenGL ¯\_(ツ)_/¯

	// Sort the primitives and retrieve the contour
	vector<vector<Point>> contour = getContour(contourTexData, fieldWidthPerm, fieldHeightPerm);
	// Draw the contour
	srand(glfwGetTime());
	renderer.renderContour(true, contour, GL_LINE_STRIP);

	// Find candidate positions for the contour
	vector<vector<GLfloat>> angles = Labeler::computeCurvatureAngles(contour);
	vector<vector<Point>> finalCandidatePositions;
	finalCandidatePositions.reserve(contour.size());
	// Construct labels
	const Labeler::Label label(numChars, charWidth, charHeight);
	vector<Labeler::Label> labels(contour.size(), label);
	// Look for candidate positions
	// Turn on wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)

	// For each contourLine 
	for (int i = 0; i < contour.size(); i++) {
		// Find candidate positions
		/*renderer.render2Dsmth(&contour[i][0], contour[i].size(), GL_LINE_STRIP, glm::vec3(.1f, 1.f, .1f));
		glfwSwapBuffers(window);*/
		vector<Labeler::CandidatePosition> candidatePositions = Labeler::findCandidatePositions(label.getTotalWidth(), label.charHeight, contour[i], angles[i]);
		//glfwSwapBuffers(window);
		// Sort by ascending curvature
		std::sort(candidatePositions.begin(), candidatePositions.end(), [](const Labeler::CandidatePosition& a, const Labeler::CandidatePosition& b) {return a.curvature < b.curvature; });

		// TODO: extract into a separate method
		bool candidatePositionWasFound = false;
		Labeler::Label positionedLabel;
		// For every candidatePosition
		for (const Labeler::CandidatePosition& candidatePosition : candidatePositions) {
			// Place label onto candidatePosition
			positionedLabel = label;
			positionedLabel.straight = candidatePosition.straight;
			Labeler::positionLabelOnLine(positionedLabel, candidatePosition.position);

			// Get ready for pre-culling
			positionedLabel.determineAABB(); // TODO: this method should not be accessible by users			

			// Check for intersections
			if (!Labeler::intersect(addedLabels, positionedLabel, renderer)) {
				candidatePositionWasFound = true;

				// ~Debug
				finalCandidatePositions.push_back(candidatePosition.position);

				addedLabels.push_back(positionedLabel);

				break;
			}
			else {
				continue;
			}
		}

		// If all candidate positions intersect with something
		if (!candidatePositionWasFound &&  candidatePositions.size() > 0 /*happens when debugging and writing smth into the contour image*/) {
			positionedLabel = label;
			Labeler::positionLabelOnLine(positionedLabel, candidatePositions[0].position);
			finalCandidatePositions.push_back(candidatePositions[0].position);
			addedLabels.push_back(positionedLabel);
		}
	}

	// Turn off wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Debug
	// Render the overlapping contours
	/*for (Labeler::Label label : Labeler::overlappingLabels) {
	renderLabel(Labeler::labelToPositionsArray(label));
	}*/

	/*cout << "debug (glm::vec2(10.f, 5.f) / glm::vec2(20.f, 20.f) - .5f)*2.f: " << ((glm::vec2(10.f, 5.f) / glm::vec2(20.f, 20.f) - .5f)*2.f).x << ", " << ((glm::vec2(10.f, 5.f) / glm::vec2(20.f, 20.f) - .5f)*2.f).y;
	glfwSwapBuffers(window);
	glfwSwapBuffers(window);*/

	// May crash because of the case described in findCandidatePositions TODO
	// Draw candidate positions
	//renderer.renderContour(true, finalCandidatePositions, GL_LINE_STRIP);
	//glfwSwapBuffers(window);
	//glfwSwapBuffers(window);
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
	GLfloat* scalarField = nullptr;
	GLint* fieldCoords = nullptr;
	GLint fieldCoordsSize; // TODO: get rid of
	generateScalarField(scalarField, fieldWidthPerm, fieldHeightPerm, xDomain.x, yDomain.x, xDomain.y, yDomain.y, fieldCoords, fieldCoordsSize);
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
	//delete[] scalarField;

	// Setup the output image for the texture
	glActiveTexture(GL_TEXTURE1);
	GLuint contourTex;
	//GLfloat dummyArray[2]{ dummyValue, dummyValue };
	glGenTextures(1, &contourTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG32F, fieldWidth, fieldHeight, 4);
	/*glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyArray[0]);*/
	glBindImageTexture(1, contourTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	// Set uniforms
	glUseProgram(computeProgram);
	// Image units uniforms
	glUniform1i(glGetUniformLocation(computeProgram, "scalarField"), 0);
	glUniform1i(glGetUniformLocation(computeProgram, "contour"), 1);


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Renderer renderer;

	measure_start(1)
	produceLabeledContour(5, .05f, .1f, .5f, computeProgram, contourTex, window, renderer);
	produceLabeledContour(4, .05f, .1f, .4f, computeProgram, contourTex, window, renderer);
	produceLabeledContour(5, .05f, .1f, .9f, computeProgram, contourTex, window, renderer);
	measure_end
		/*glfwSwapBuffers(window);
		glfwSwapBuffers(window);*/

		for (Labeler::Label label : addedLabels) {
			renderer.renderLabel(Labeler::labelToPositionsArray(label));
		}
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
void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize) {
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

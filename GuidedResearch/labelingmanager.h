#pragma once

#include "shader.h"
#include <SOIL.h>

#include <iomanip>      // std::setprecision
#include <algorithm>

#include "contourer.h"
#include "labeler.h"
#include "renderer.h"

class LabelingManager {
private:
	Contourer contourer;
	Renderer renderer;
	GLfloat* scalarField;
	GLint fieldWidth;
	GLint fieldHeight;
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	GLint fieldWidthPerm;// = fieldHeight;
	// Value we'd need to use after retrieving the image from OpenGL (column-major) to C++ (row-major).
	GLint fieldHeightPerm;// = fieldWidth;

	GLuint computeProgram;
	GLuint contourTex;
	GLuint scalarFieldTex;

	vector<vector<vector<Point>>> contours;
	vector<Labeler::Label> addedLabels;


	void setFieldWidth(const GLint fieldWidth) {
		this->fieldWidth = fieldWidth;
		this->fieldHeightPerm = fieldWidth;
	}

	void setFieldHeight(const GLint fieldHeight) {
		this->fieldHeight = fieldHeight;
		this->fieldWidthPerm = fieldHeight;
	}

	void setup() {
		// Setup compute shader
		GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
		string computeSrcString = Shader::getShaderCode("shaders/marchingsquares/marchingsquares.comp");
		const GLchar* computeSrc = computeSrcString.c_str();
		glShaderSource(computeShader, 1, &computeSrc, NULL);
		glCompileShader(computeShader);
		Shader::checkShaderCompilationStatus(computeShader, computeSrc);
		// Setup compute program
		computeProgram = glCreateProgram();
		glAttachShader(computeProgram, computeShader);
		glLinkProgram(computeProgram);
		Shader::checkProgramLinkingStatus(computeProgram);
		// Delete shader
		glDeleteShader(computeShader);

		// Scalar Field setup
		//GLfloat* scalarField = nullptr; // TODO: don't forget to remove in the end
		/*generateScalarField(scalarField, fieldWidthPerm, fieldHeightPerm, xDomain.x, yDomain.x, xDomain.y, yDomain.y);*/
		//generateScalarField(scalarField, fieldWidth, fieldHeight, -8, -3, 8, 3, fieldCoords, fieldCoordsSize);

		// Store scalar field into a texture
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &scalarFieldTex);
		glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
		glBindImageTexture(0, scalarFieldTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Setup the output image for the texture
		glActiveTexture(GL_TEXTURE1);
		glGenTextures(1, &contourTex);
		glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG32F, fieldWidth, fieldHeight, 4, 0, GL_RG, GL_FLOAT, nullptr); // TODO: change to glTexImage3D
		glBindImageTexture(1, contourTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		// Set uniforms
		glUseProgram(computeProgram);
		// Image units uniforms
		glUniform1i(glGetUniformLocation(computeProgram, "scalarField"), 0);
		glUniform1i(glGetUniformLocation(computeProgram, "contour"), 1);
	}

public:
	LabelingManager(GLint fieldWidth, GLint fieldHeight, GLfloat* scalarField) : scalarField(scalarField), contourer(fieldHeight, fieldWidth) {
		setFieldWidth(fieldWidth);
		setFieldHeight(fieldHeight);
		setup();
	}

	void produceLabeledContour(const int& numChars, const GLfloat& charWidth, const GLfloat& charHeight, GLfloat isoValue)
	{
		GLfloat dummyArray[2]{ dummyValue, dummyValue };
		glClearTexImage(contourTex, 0, GL_RG, GL_FLOAT, &dummyArray[0]);

		glUseProgram(computeProgram);
		glUniform1f(glGetUniformLocation(computeProgram, "isoValue"), isoValue);
		// TODO: check for zeroes
		glUniform2f(glGetUniformLocation(computeProgram, "domainUpperBound"), static_cast<GLfloat>(fieldWidth - 1), static_cast<GLfloat>(fieldHeight - 1));

		// COMPUTER SHADER
		glDispatchCompute(fieldWidth - 1, fieldHeight - 1, 1);
		// Ensure that writes by the compute shader have completed
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Read back the 2D array texture
		vector<GLfloat> contourTexData((fieldWidthPerm * 4)*(fieldHeightPerm * 2)); // might be better to store on heap

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RG, GL_FLOAT, &contourTexData[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		// Sort the primitives and retrieve the contour
		vector<vector<Point>> contour = contourer.getContour(contourTexData);
		// Draw the contour
		//srand(glfwGetTime());
		//renderer.renderContour(true, contour, GL_LINE_STRIP);

		// Find candidate positions for the contour
		vector<vector<GLfloat>> angles = Labeler::computeCurvatureAngles(contour);
		// Construct labels
		const Labeler::Label label(numChars, charWidth, charHeight);
		vector<Labeler::Label> labels(contour.size(), label);
		// Look for candidate positions
		// Turn on wireframe mode
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)

		// For each contourLine 
		for (int i = 0; i < contour.size(); i++) {
			// Find all(!) candidate positions. TODO: make a more sophisticated choice
			vector<Labeler::CandidatePosition> candidatePositions = Labeler::findCandidatePositions(label.getTotalWidth(), label.charHeight, contour[i], angles[i]);
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
					
					addedLabels.push_back(positionedLabel);

					break;
				}
				else {
					continue;
				}
			}

			// If all candidate positions intersect with something
			if (!candidatePositionWasFound &&  candidatePositions.size() > 0 /*latter happens when debugging and writing smth into the contour image*/) {
				// Choose the first candidate position. TODO: make more sophisticated choice.
				positionedLabel = label;
				positionedLabel.straight = candidatePositions[0].straight;
				Labeler::positionLabelOnLine(positionedLabel, candidatePositions[0].position);
				positionedLabel.determineAABB();
				addedLabels.push_back(positionedLabel);

			}
		}

		contours.push_back(std::move(contour));
	}

	void renderLabels() {
		// Separate method
		for (Labeler::Label label : addedLabels) {
			renderer.renderLabel(Labeler::labelToPositionsArray(label));
		}
	}

	void renderContours() {
		for (vector<vector<Point>> contour : contours) {
			renderer.renderContour(true, contour, GL_LINE_STRIP);
		}
	}

	// Clears accumulated label and contour data. If you need to clear the screen, use glClear as you normally would.
	void clearData() {
		contours.clear(); 
		addedLabels.clear();
	}

	// Swap scalar field for a different scalar field of the same size.
	void swapScalarField(GLfloat* scalarField) {
		this->scalarField = scalarField;
	}

	// Swap scalar field for a different scalar field of a different size.
	void updateScalarFieldSize(GLfloat* scalarField, const GLint newFieldWidth, const GLint newFieldHeight) {
		clearData();

		this->scalarField = scalarField;
		setFieldWidth(newFieldWidth);
		setFieldHeight(newFieldHeight);

		contourer.fieldWidth = newFieldWidth;
		contourer.fieldHeight = newFieldHeight;

		// Update scalar field texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Update the output image for the texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, contourTex);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG32F, fieldWidth, fieldHeight, 4, 0, GL_RG, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
};

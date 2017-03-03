#pragma once

#include <GLFW\glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdlib>

#include <util/Shader.h>

#include "point.h"

using namespace std;

void renderContour(const bool& randColorsPerContourLine, vector<vector<Point>>& contour, const int& primitiveEnumType)
{
	Shader shader("shaders/reconstructedcontour/reconstructedcontour.vert", "shaders/reconstructedcontour/reconstructedcontour.frag", "");

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	GLfloat colors[3]{ 1.f, 1.f, 1.f };
	for (int i = 0; i < contour.size(); i++)
	{
		cout << "contour[" << i << "].size(): " << contour[i].size() << endl;

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, contour[i].size() * 2 * sizeof(GLfloat), &contour[i][0], GL_STATIC_DRAW);

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

		glLineWidth(5.f);

		shader.Use();
		if (randColorsPerContourLine) {
			colors[0] = rand() / (GLfloat)RAND_MAX;
			colors[1] = rand() / (GLfloat)RAND_MAX;
			colors[2] = rand() / (GLfloat)RAND_MAX;
		}
		glUniform3f(glGetUniformLocation(shader.Program, "u_Color"), colors[0], colors[1], colors[2]);
		glDrawArrays(primitiveEnumType, 0, contour[i].size());
		glBindVertexArray(0);
	}
	// Clean-up
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

// Debug only
template <size_t rows>
void renderLabelCharacter(const Point (&charPoints)[rows])
{
	Shader shader("shaders/renderer/char.vert", "shaders/renderer/char.frag", "");

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, rows * sizeof(GLfloat) * 2, &charPoints[0], GL_STATIC_DRAW);

	glBindVertexArray(VAO);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

	shader.Use();
	//glUniformMatrix3fv(glGetUniformLocation(shader.Program, "mvp"), 1, GL_FALSE, &mvp[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, rows);
	glBindVertexArray(0);

	// Clean-up
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

void renderLabel(const vector<Point>& labelPositionsArray)
{
	Shader shader("shaders/renderer/char.vert", "shaders/renderer/char.frag", "");

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, labelPositionsArray.size() * sizeof(GLfloat) * 2, &labelPositionsArray[0], GL_STATIC_DRAW);

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

		shader.Use();
		//glUniformMatrix3fv(glGetUniformLocation(shader.Program, "mvp"), 1, GL_FALSE, &mvp[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, labelPositionsArray.size());
		glBindVertexArray(0);

	// Clean-up
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}
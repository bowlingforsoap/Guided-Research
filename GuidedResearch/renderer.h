#pragma once

#include <GLFW\glfw3.h>
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
	GLfloat colors[3]{ rand() / (GLfloat)RAND_MAX, rand() / (GLfloat)RAND_MAX, rand() / (GLfloat)RAND_MAX };
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

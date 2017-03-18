#pragma once

#include <GLFW\glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>

#include <util/Shader.h>

#include "point.h"

using namespace std;

class Renderer {
private:
	Shader reconstructedContourShader;
	Shader charShader;
	GLuint reconstructedContourVAO, reconstructedContourVBO;
	GLuint charVAO, charVBO;

public:
	Renderer() : reconstructedContourShader("shaders/reconstructedcontour/reconstructedcontour.vert", "shaders/reconstructedcontour/reconstructedcontour.frag", ""), charShader("shaders/renderer/char.vert", "shaders/renderer/char.frag", "") {

		glGenBuffers(1, &reconstructedContourVBO);
		glGenVertexArrays(1, &reconstructedContourVAO);

		glGenBuffers(1, &charVBO);
		glGenVertexArrays(1, &charVAO);

	} 

	~Renderer() {
		glDeleteBuffers(1, &reconstructedContourVBO);
		glDeleteVertexArrays(1, &reconstructedContourVAO);

		glDeleteBuffers(1, &charVBO);
		glDeleteVertexArrays(1, &charVAO);
	}

	void renderContour(const bool& randColorsPerContourLine, vector<vector<Point>>& contour, const int& primitiveEnumType)
	{
		/*Shader shader("shaders/reconstructedcontour/reconstructedcontour.vert", "shaders/reconstructedcontour/reconstructedcontour.frag", "");

		GLuint VBO, VAO;
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);*/
		glm::vec3 colors(1.f, 1.f, 1.f);
		for (int i = 0; i < contour.size(); i++)
		{
			//cout << "contour[" << i << "].size(): " << contour[i].size() << endl;

			glBindBuffer(GL_ARRAY_BUFFER, reconstructedContourVBO);
			glBufferData(GL_ARRAY_BUFFER, contour[i].size() * 2 * sizeof(GLfloat), &contour[i][0], GL_STATIC_DRAW);

			glBindVertexArray(reconstructedContourVAO);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);


			reconstructedContourShader.Use();
			if (randColorsPerContourLine) {
				colors = genRandColor();
			}
			glUniform3f(glGetUniformLocation(reconstructedContourShader.Program, "u_Color"), colors[0], colors[1], colors[2]);

			glLineWidth(5.f);
			glDrawArrays(primitiveEnumType, 0, contour[i].size());
			glLineWidth(1.f);
			glBindVertexArray(0);
		}
		// Clean-up
		/*glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);*/
	}

	// Debug only
	template <size_t rows>
	void renderLabelCharacter(const Point(&charPoints)[rows])
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
		/*Shader shader("shaders/renderer/char.vert", "shaders/renderer/char.frag", "");

		GLuint VBO, VAO;
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);*/

		glBindBuffer(GL_ARRAY_BUFFER, charVBO);
		glBufferData(GL_ARRAY_BUFFER, labelPositionsArray.size() * sizeof(GLfloat) * 2, &labelPositionsArray[0], GL_STATIC_DRAW);

		glBindVertexArray(charVAO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

		charShader.Use();
		//glUniformMatrix3fv(glGetUniformLocation(shader.Program, "mvp"), 1, GL_FALSE, &mvp[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, labelPositionsArray.size());
		glBindVertexArray(0);

		// Clean-up
		//glDeleteBuffers(1, &VBO);
		//glDeleteVertexArrays(1, &VAO);
	}

	// Debug only
	void render2Dsmth(const Point* points, const size_t& numPoints, const int& primitiveType, const bool& randColor)
	{
		Shader shader("shaders/renderer/2Dsmth.vert", "shaders/renderer/2Dsmth.frag", "");

		GLuint VBO, VAO;
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, numPoints * sizeof(GLfloat) * 2, &points[0], GL_STATIC_DRAW);

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);


		shader.Use();
		glm::vec3 color(1.f, 1.f, 1.f);
		if (randColor) {
			color = genRandColor();
		}
		glUniform3fv(glGetUniformLocation(shader.Program, "u_Color"), 1, glm::value_ptr(color));

		glLineWidth(5.f);
		glDrawArrays(primitiveType, 0, numPoints);
		glLineWidth(1.f);
		glBindVertexArray(0);

		// Clean-up
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}

	private:
		inline glm::vec3 genRandColor() {
			glm::vec3 colors;

			srand(glfwGetTime());
			colors[0] = rand() / (GLfloat)RAND_MAX;
			colors[1] = rand() / (GLfloat)RAND_MAX;
			colors[2] = rand() / (GLfloat)RAND_MAX;

			return colors;
		}

};

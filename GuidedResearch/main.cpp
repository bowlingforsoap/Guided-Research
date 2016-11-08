#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

GLfloat* generateScalarField(GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY) {
	GLfloat* scalarFiled = new float[width * height];
	float xDelta = abs(maxX - minX) / width;
	float yDelta = abs(maxY - minY) / height;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			scalarFiled[i * height + j] = sin(minX + xDelta * i) * cos(minY + yDelta * j);
		}
	}

	return scalarFiled;
}

int main() {
	GLint width = 1600, height = 1000;
	GLFWwindow* window = glfwInitialize(width, height, "Guided Research", 4, 2, false);
	glewInit();
	glViewport(0, 0, width, height);

	// Setup shaders
	Shader shader("vert.glsl", "frag.glsl", "geometry.glsl");

	// Scalar Field setup
	GLint fieldWidth = 600, fieldHeight = 600;
	GLfloat* scalarField = generateScalarField(fieldWidth, fieldHeight, -3, -2, 3, 2);
	// Store scalar field into a texture
	GLuint scalarFieldTex;
	glGenTextures(1, &scalarFieldTex);
	glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
	glBindTexture(GL_TEXTURE_2D, 0);
	cout << glGetError() << endl;
	delete[] scalarField;
	// Set texture uniform
	shader.Use();
	glUniform1i(glGetUniformLocation(shader.Program, "scalarField"), 0);

	// Points to render
	GLfloat points[] {
		0.f, .0f, 0.f
	};

	// Setup attributes
	//
	GLuint VAO, VBO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	// Bind data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points, GL_STATIC_DRAW);
	// Setup vertex array
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindVertexArray(0);


	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	// Gameloop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);

		shader.Use();
		glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, 1);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 666;
}
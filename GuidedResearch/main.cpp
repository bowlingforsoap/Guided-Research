#include <util/GLFWinitializer.h>
#include <util/Shader.h>

int main() {
	GLint width = 1600, height = 1000;
	GLFWwindow* window = glfwInitialize(width, height, "Guided Research", 4, 2, false);
	glewInit();
	glViewport(0, 0, width, height);

	// Setup shaders
	Shader shader("vert.glsl", "frag.glsl", "geometry.glsl");

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
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, 1);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 666;
}
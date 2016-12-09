#version 420 core

in layout(location = 0) vec2 position;
out vec4 vs_Feedback;

void main() {
	gl_Position = vec4(position, 0.f, 1.f);
	vs_Feedback = gl_Position;
}

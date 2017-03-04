#version 420 core

out vec4 color;

//uniform mat3 mvp;
// in vec3 vs_Color;

void main() {
  // color = vec4(vs_Color, 1.f);
  color = vec4(1.f, 1.f, 1.f, 1.f);
}

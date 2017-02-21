#version 420 core

out vec4 color;

uniform vec3 u_Color;

void main() {
  color = vec4(u_Color, 1.f);
  // color = vec4(1.f, 1.f, 1.f, 1.f);
}

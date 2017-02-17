#version 420 core

//uniform vec3 u_Color;
out vec4 color;

void main() {
  //color = vec4(u_Color, 1.f);
  color = vec4((vec2(gl_FragCoord) + 1) / 2, 0.f, 1.f);
}

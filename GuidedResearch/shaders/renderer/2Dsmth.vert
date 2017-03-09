#version 420 core

in layout(location = 1) vec2 position;

void main() {
  gl_Position = vec4(position, 0.f, 1.f);
}

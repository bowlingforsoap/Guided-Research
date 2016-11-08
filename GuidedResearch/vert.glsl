#version 420 core

in layout(location = 0) ivec2 scalarFieldCoords;

void main() {
  // Store coordinates in gl_Position.xy
  gl_Position = vec4(scalarFieldCoords, 0.f, 1.f);
}

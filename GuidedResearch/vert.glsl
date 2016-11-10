#version 420 core

//in layout(location = 0) ivec2 scalarFieldCoords;
in layout(location = 0) vec2 position;
in layout(location = 1) vec2 texCoords;

out VS_OUT {
  vec2 texCoords;
} vs_out;

void main() {
  // Store coordinates in gl_Position.xy
  gl_Position = vec4(position, 0.f, 1.f);
  vs_out.texCoords = texCoords;
}

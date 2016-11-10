#version 420 core

in VS_OUT {
  vec2 texCoords;
} fs_in;
out vec4 color;

uniform sampler2D scalarFieldTex;

void main() {
  color = texture(scalarFieldTex, fs_in.texCoords);
}

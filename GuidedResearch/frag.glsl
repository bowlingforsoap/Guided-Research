#version 420 core

in VS_OUT {
  vec2 texCoords;
} fs_in;
out vec4 color;

uniform sampler2D scalarFieldTex;
layout (binding = 0, r32f) uniform image2D scalarField;

void main() {
  //color = texture(scalarFieldTex, fs_in.texCoords);
  // 100 - hardcoded image size
  color = imageLoad(scalarField, ivec2(fs_in.texCoords * 100));
}

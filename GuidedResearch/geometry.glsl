#version 420 core

layout (points) in;
layout (line_strip, max_vertices = 4) out;

layout (binding = 0, r32f) uniform image2D scalarField;

void main() {
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = gl_in[0].gl_Position + vec4(.3f, .3f, 0.f, 0.f);
  EmitVertex();

  EndPrimitive();
}

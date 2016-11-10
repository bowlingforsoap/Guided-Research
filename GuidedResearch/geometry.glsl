#version 420 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (binding = 0, r32f) uniform image2D scalarField;
uniform float isoValue;

in VS_OUT {
  vec2 texCoords;
} gs_in[];

out vec2 texCoords;

/**
  Emit a vertex between the two edge points p1 and p2 of the currently
  processed grid cell. This function needs to be called twice for each line
  segment to be generated.

  Parameters are the world space position of p1 and p2, their intensities,
  and the iso value of the current contour line.
 */
/*void emit(vec2 p1_NDCposition, vec2 p2_NDCposition,
          float p1_intensity, float p2_intensity, float isoValue)
{
  float fraction = (isoValue - p1_intensity) / (p2_intensity - p1_intensity);
  vec2 vertex_NDCposition = mix(p1_NDCposition, p2_NDCposition, fraction);
  vec4(vertex_NDCposition, 0.f, 1.f);
  EmitVertex();
}*/

void main() {
  for (int i = 0; i < gl_in.length(); i++) {
      gl_Position = gl_in[i].gl_Position;
      texCoords = gs_in[i].texCoords;
      EmitVertex();
  }
  EndPrimitive();
  // gl_Position.xy encodes the i and j grid indices of the upper left corner
  // of the grid cell processed by this shader instance.
  /*ivec2 ul = ivec2(gl_in[0].gl_Position.xy);
  ivec2 ur = ul + ivec2(1, 0);
  ivec2 ll = ul + ivec2(0, 1);
  ivec2 lr = ur + ivec2(0, 1);

  // Load the scalar values at the four corner points of the grid cell.
  float ul_intensity = imageLoad(scalarField, ul).r;
  float ur_intensity = imageLoad(scalarField, ur).r;
  float ll_intensity = imageLoad(scalarField, ll).r;
  float lr_intensity = imageLoad(scalarField, lr).r;

  // .. and the NDC coordinates of these corner points.
  // TODO: unhardcore 100
  vec2 ul_NDCposition = ((ul - vec2(0)) / (vec2(100) - vec2(0)) - .5f) * 2;
  vec2 ur_NDCposition = ((ur - vec2(0)) / (vec2(100) - vec2(0)) - .5f) * 2;
  vec2 ll_NDCposition = ((ll - vec2(0)) / (vec2(100) - vec2(0)) - .5f) * 2;
  vec2 lr_NDCposition = ((lr - vec2(0)) / (vec2(100) - vec2(0)) - .5f) * 2;

  // Determine the marching squares case we are handling..
  int bitfield = 0;
  bitfield += int(ll_intensity < isoValue) * 1;
  bitfield += int(lr_intensity < isoValue) * 2;
  bitfield += int(ul_intensity < isoValue) * 4;
  bitfield += int(ur_intensity < isoValue) * 8;
  // ..use symmetry.
  if (bitfield > 7)
  {
      bitfield = 15 - bitfield;
  }

  // Emit vertices according to the case determined above.
  if (bitfield == 0)
  {
      EndPrimitive();
  }
  else if (bitfield == 1)
  {
      emit(ll_NDCposition, ul_NDCposition, ll_intensity, ul_intensity, isoValue);
      emit(ll_NDCposition, lr_NDCposition, ll_intensity, lr_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 2)
  {
      emit(ll_NDCposition, lr_NDCposition, ll_intensity, lr_intensity, isoValue);
      emit(lr_NDCposition, ur_NDCposition, lr_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 3)
  {
      emit(ll_NDCposition, ul_NDCposition, ll_intensity, ul_intensity, isoValue);
      emit(lr_NDCposition, ur_NDCposition, lr_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 4)
  {
      emit(ll_NDCposition, ul_NDCposition, ll_intensity, ul_intensity, isoValue);
      emit(ul_NDCposition, ur_NDCposition, ul_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 5)
  {
      emit(ll_NDCposition, lr_NDCposition, ll_intensity, lr_intensity, isoValue);
      emit(ul_NDCposition, ur_NDCposition, ul_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 6)
  {
      emit(ll_NDCposition, ul_NDCposition, ll_intensity, ul_intensity, isoValue);
      emit(ll_NDCposition, lr_NDCposition, ll_intensity, lr_intensity, isoValue);
      EndPrimitive();
      emit(ul_NDCposition, ur_NDCposition, ul_intensity, ur_intensity, isoValue);
      emit(lr_NDCposition, ur_NDCposition, lr_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }
  else if (bitfield == 7)
  {
      emit(ul_NDCposition, ur_NDCposition, ul_intensity, ur_intensity, isoValue);
      emit(lr_NDCposition, ur_NDCposition, lr_intensity, ur_intensity, isoValue);
      EndPrimitive();
  }*/

}

#version 420 core

in layout(location = 1) vec2 position;

out vec3 vs_Color;

// Random value generator.
float random(vec2 p){return fract(cos(dot(p,vec2(23.14069263277926,2.665144142690225)))*123456.);}

void main() {
  vs_Color = (vec3(random(position), random(position.yx), random(vec2(position.x, 0.f))) + 1.f) / 2.f;

  gl_Position = vec4(position, 0.f, 1.f);
}

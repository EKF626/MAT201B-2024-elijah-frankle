#version 400

uniform vec3 secondColor;

in Fragment {
  vec4 color;
  vec2 mapping;
}
fragment;

layout(location = 0) out vec4 fragmentColor;

void main() {
  float blendAmt = clamp(r, 0, 1);
  vec3 newColor = mix(fragment.color.rgb, secondColor, blendAmt);
  fragmentColor = vec4(newColor, fragment.color.a);
}
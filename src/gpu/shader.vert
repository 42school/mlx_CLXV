
#version 460

layout(binding = 0) uniform uniform_object {
    vec2 src_full;
    vec2 src_pos;
    vec2 src_size;
    vec2 dst_full;
    vec2 dst_pos;
    vec2 dst_size;
    vec4 color;
} uni;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec4 color;

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);


void main()
{
  vec2 pos;
  pos = (uni.dst_pos + positions[gl_VertexIndex] * uni.dst_size) / uni.dst_full;
  pos = (pos * 2.0) - vec2(1.0, 1.0);
  gl_Position = vec4(pos[0], pos[1], 0.0, 1.0);
  UV = (uni.src_pos + positions[gl_VertexIndex] * uni.src_size) / uni.src_full;
  color = uni.color;
}


#version 460

layout(location = 0) in vec2 UV;
layout(location = 1) in vec4 color;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = color*texture(texSampler, UV);
}

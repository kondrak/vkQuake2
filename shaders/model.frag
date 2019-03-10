#version 450

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in flat int textured;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    if(textured != 0)
        fragmentColor = texture(sTexture, texCoord) * clamp(color, 0.0, 1.0);
    else
        fragmentColor = color;
}

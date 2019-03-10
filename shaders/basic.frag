#version 450

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 color;
layout(location = 2) in float aTreshold;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = texture(sTexture, texCoord) * color;
    if(fragmentColor.a < aTreshold)
        discard;
}

#version 450

layout(set = 1, binding = 0) uniform sampler2D sTexture;
layout(set = 2, binding = 0) uniform sampler2D sLightmap;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 texCoordLmap;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = texture(sTexture, texCoord) * texture(sLightmap, texCoordLmap);
}

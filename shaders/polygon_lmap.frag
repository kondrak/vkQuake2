#version 450

layout(set = 0, binding = 0) uniform sampler2D sTexture;
layout(set = 2, binding = 0) uniform sampler2D sLightmap;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 texCoordLmap;
layout(location = 2) in float viewLightmaps;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 color = texture(sTexture, texCoord);
    vec4 light = texture(sLightmap, texCoordLmap);
    fragmentColor = (1.0 - viewLightmaps) * color * light + viewLightmaps * light;
}

#version 450

layout(binding = 1) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(texCoord.x, texCoord.y, 0.0, 1.0);
    //fragmentColor = texture(sTexture, texCoord);
}

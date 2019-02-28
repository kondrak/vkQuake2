#version 450

layout(set = 1, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 color = texture(sTexture, texCoord);
    if(color.a < 0.666)
        discard;
    fragmentColor = color;
}

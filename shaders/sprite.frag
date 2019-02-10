#version 450

layout(set = 1, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float alpha;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = texture(sTexture, texCoord) * vec4(1.0, 1.0, 1.0, alpha);
    if(fragmentColor.a < 0.0666)
        discard;
}

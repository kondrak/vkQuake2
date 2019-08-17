#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragmentColor;

void main() 
{
	fragmentColor = texture(sTexture, texCoord);
}

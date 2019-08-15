#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D sTexture;
layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput color_input;

layout(location = 0) in vec2 texCoord;
layout (location = 0) out vec4 fragmentColor;

void main() 
{
	fragmentColor = texture(sTexture, texCoord); //subpassLoad(color_input);
}

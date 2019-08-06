#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput color_input;

layout (location = 0) out vec4 fragmentColor;

void main() 
{
	fragmentColor = subpassLoad(color_input);
}

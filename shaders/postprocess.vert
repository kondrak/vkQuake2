#version 450
#extension GL_ARB_separate_shader_objects : enable

// rendering a fullscreen quad (which is actually just a huge triangle)
// source: https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec2 texCoord;

void main() 
{
	texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}

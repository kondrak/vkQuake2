#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 mvpMatrix;
    vec4 color;
} ubo;

layout(location = 0) out vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.mvpMatrix * vec4(inVertex, 1.0);
    color = ubo.color;
}

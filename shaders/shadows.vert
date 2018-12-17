#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 mvpMatrix;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.mvpMatrix * vec4(inVertex, 1.0);
}

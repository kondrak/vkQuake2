#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 0) out vec4 color;

layout(push_constant) uniform PushConstant
{
    mat4 vpMatrix;
} pc;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = pc.vpMatrix * ubo.model * vec4(inVertex, 1.0);
    color = vec4(0.0, 0.0, 0.0, 0.5);
}

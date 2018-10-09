#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 ModelViewProjectionMatrix;
} ubo;

layout(location = 0) in vec3 inVertex;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.ModelViewProjectionMatrix * vec4(inVertex, 1.0);
}

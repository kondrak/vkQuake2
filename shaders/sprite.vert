#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 mvpMatrix;
    float alpha;
} ubo;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float alpha;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.mvpMatrix * vec4(inVertex, 1.0);
    texCoord = inTexCoord;
    alpha = ubo.alpha;
}

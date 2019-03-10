#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;

layout(push_constant) uniform PushConstant
{
    mat4 vpMatrix;
} pc;

layout(set = 1, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    vec4 color;
    float time;
    float scroll;
} ubo;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 color;
layout(location = 2) out float aTreshold;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = pc.vpMatrix * ubo.model * vec4(inVertex, 1.0);
    texCoord = inTexCoord + vec2(sin(2.0 * ubo.time + inTexCoord.y * 3.28), sin(2.0 * ubo.time + inTexCoord.x * 3.28)) * 0.05;
    texCoord.x += ubo.scroll;
    color = ubo.color;
    aTreshold = 0.0;
}

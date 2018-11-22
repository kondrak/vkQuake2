#version 450
#extension GL_ARB_separate_shader_objects : enable

// normalized offset and scale
layout(set = 0, binding = 0) uniform imageTransform
{
    vec2 offset;
    vec2 scale;
    vec4 color;
} it;

layout(location = 0) in vec2 inVertex;

layout(location = 0) out vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec2 vPos = inVertex.xy * it.scale - (vec2(1.0) - it.scale);
    gl_Position = vec4(vPos + it.offset * 2.0, 0.0, 1.0);
    color = it.color;
}

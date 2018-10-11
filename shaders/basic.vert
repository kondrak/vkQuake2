#version 450
#extension GL_ARB_separate_shader_objects : enable

// normalized offset and scale
layout(binding = 0) uniform imageTransform
{
    vec2 offset;
    vec2 scale;
} it;

layout(location = 0) in vec3 inVertex;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec2 vPos = inVertex.xy * it.scale - (vec2(1.0) - it.scale);
    gl_Position = vec4(vPos + it.offset * 2.0, 0.0, 1.0);
}

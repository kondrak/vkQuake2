#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;

layout(push_constant) uniform PushConstant
{
    mat4 vpMatrix;
    mat4 model;
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = pc.vpMatrix * pc.model * vec4(inVertex, 1.0);
}

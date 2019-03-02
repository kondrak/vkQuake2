#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;

layout(push_constant) uniform PushConstant
{
    mat4 mvpMatrix;
    float alpha;
} pc;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float alpha;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = pc.mvpMatrix * vec4(inVertex, 1.0);
    texCoord = inTexCoord;
    alpha = pc.alpha;
}

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec4 inColor;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 mvpMatrix;
    float pointSize;
    float minPointSize;
    float maxPointSize;
    float att_a;
    float att_b;
    float att_c;
} ubo;

layout(location = 0) out vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main() {
    gl_Position = ubo.mvpMatrix * vec4(inVertex, 1.0);
    float dist_atten = 1.0 / (ubo.att_a + ubo.att_b * gl_Position.w + ubo.att_c * gl_Position.w * gl_Position.w);
    gl_PointSize = ubo.pointSize * sqrt(dist_atten);

    if(gl_PointSize < ubo.minPointSize)
        gl_PointSize = ubo.minPointSize;

    if(gl_PointSize > ubo.maxPointSize)
        gl_PointSize = ubo.maxPointSize;

    color = inColor;
}

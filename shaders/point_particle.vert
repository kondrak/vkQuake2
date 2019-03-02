#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstant
{
    mat4 mvpMatrix;
    float pointSize;
    float pointScale;
    float minPointSize;
    float maxPointSize;
    float att_a;
    float att_b;
    float att_c;
} pc;

layout(location = 0) out vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main() {
    gl_Position = pc.mvpMatrix * vec4(inVertex, 1.0);
    float dist_atten = pc.pointScale / (pc.att_a + pc.att_b * gl_Position.w + pc.att_c * gl_Position.w * gl_Position.w);
    gl_PointSize = pc.pointScale * pc.pointSize * sqrt(dist_atten);

    if(gl_PointSize < pc.minPointSize)
        gl_PointSize = pc.minPointSize;

    if(gl_PointSize > pc.maxPointSize)
        gl_PointSize = pc.maxPointSize;

    color = inColor;
}

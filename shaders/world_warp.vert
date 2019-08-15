#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(push_constant) uniform PushConstant
{
    float time;
	float scrWidth;
	float scrHeight;
} pc;

layout(location = 0) out float iTime;
layout(location = 1) out vec2 screenRes;

void main() 
{
	vec4 positions[3] = {
		vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		vec4(3.0f, -1.0f, 0.0f, 1.0f),
		vec4(-1.0f, 3.0f, 0.0f, 1.0f)
	};

	gl_Position = positions[gl_VertexIndex % 3];
    iTime = pc.time;
	screenRes = vec2(pc.scrWidth, pc.scrHeight);
}

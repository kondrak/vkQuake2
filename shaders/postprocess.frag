#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant
{
	float postprocess;
	float gamma;
} pc;

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragmentColor;

void main() 
{
	// apply any additional world-only postprocessing effects here (if enabled)
	if (pc.postprocess > 0.0)
	{
		//gamma + color intensity bump
		fragmentColor = vec4(pow(texture(sTexture, texCoord).rgb * 1.5, vec3(pc.gamma)), 1.0);
	}
	else
	{
		fragmentColor = texture(sTexture, texCoord);
	}
}

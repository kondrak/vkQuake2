#version 450
#extension GL_ARB_separate_shader_objects : enable

// Underwater screen warp effect similar to what software renderer provides

layout(push_constant) uniform PushConstant
{
	float time;
	float scale;
	float scrWidth;
	float scrHeight;
} pc;

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fragmentColor;

#define PI 3.1415

void main()
{
	vec2 uv = vec2(gl_FragCoord.x / pc.scrWidth, gl_FragCoord.y / pc.scrHeight);

	if (pc.time > 0)
	{
		float sx = pc.scale - abs(pc.scrWidth  / 2.0 - gl_FragCoord.x) * 2.0 / pc.scrWidth;
		float sy = pc.scale - abs(pc.scrHeight / 2.0 - gl_FragCoord.y) * 2.0 / pc.scrHeight;
		float xShift = 2.0 * pc.time + uv.y * PI * 10;
		float yShift = 2.0 * pc.time + uv.x * PI * 10;
		vec2 distortion = vec2(sin(xShift) * sx, sin(yShift) * sy) * 0.00666;

		uv += distortion;
	}

	fragmentColor = texture(sTexture, uv);
}

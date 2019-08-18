#version 450
#extension GL_ARB_separate_shader_objects : enable

// Underwater screen warp effect similar to what software renderer provides

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in float iTime;
layout(location = 1) in vec3 screenInfo;

layout(location = 0) out vec4 fragmentColor;

#define PI 3.1415

void main() 
{
	vec2 uv = vec2(gl_FragCoord.x / screenInfo.x, gl_FragCoord.y / screenInfo.y);

	if (iTime > 0)
	{
		float sx = screenInfo.z - abs(screenInfo.x / 2.0 - gl_FragCoord.x) * 2.0 / screenInfo.x;
		float sy = screenInfo.z - abs(screenInfo.y / 2.0 - gl_FragCoord.y) * 2.0 / screenInfo.y;
		float xShift = 2.0 * iTime + uv.y * PI * 10;
		float yShift = 2.0 * iTime + uv.x * PI * 10;
		vec2 distortion = vec2(sin(xShift) * sx, sin(yShift) * sy) * 0.00666;

		uv += distortion;
	}

	fragmentColor = texture(sTexture, uv);
}

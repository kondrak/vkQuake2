#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D sTexture;
layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput color_input;


layout(location = 0) in float iTime;
layout(location = 1) in vec2 screenRes;

layout (location = 0) out vec4 fragmentColor;

#define speed 2.0

// the amount of shearing (shifting of a single column or row)
// 1.0 = entire screen height offset (to both sides, meaning it's 2.0 in total)
#define xDistMag 0.01
#define yDistMag 0.01

// cycle multiplier for a given screen height
// 2*PI = you see a complete sine wave from top..bottom
#define xSineCycles 6.28
#define ySineCycles 6.28

void main() 
{
    vec2 uv = vec2(gl_FragCoord.x / screenRes.x, gl_FragCoord.y / screenRes.y);
    
    float time = iTime*speed;
	
	if (time > 0)
	{
		float xAngle = time + uv.y * ySineCycles;
		float yAngle = time + uv.x * xSineCycles;
		vec2 distortOffset = vec2(sin(xAngle), sin(yAngle)) * vec2(xDistMag,yDistMag); // amount of shearing * magnitude adjustment
    
		uv += distortOffset;
	}

	fragmentColor = texture(sTexture, uv);
}

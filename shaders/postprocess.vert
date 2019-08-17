#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec2 texCoord;

void main() 
{
	vec4 positions[3] = {
		vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		vec4(3.0f, -1.0f, 0.0f, 1.0f),
		vec4(-1.0f, 3.0f, 0.0f, 1.0f)
	};

    vec2 uvs[3] = {
        vec2(-2.0f, -2.0f), vec2(0.0f, -2.0f), vec2(-2.0f, 0.0f)
    };

	//gl_Position = positions[gl_VertexIndex % 3];
    //texCoord = uvs[gl_VertexIndex % 3];
   texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}

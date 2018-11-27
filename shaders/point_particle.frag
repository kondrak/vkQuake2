#version 450

layout(location = 0) in vec4 color;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec2 cxy = 2.0 * gl_PointCoord - 1.0;
    if(dot(cxy, cxy) > 1.0)
        discard;

    fragmentColor = color;
}

#version 300 es
precision highp float;

in vec2 tex;
in float depthval;

out vec4 OutColor;

void main ()
{
	OutColor = vec4(vec3(depthval),1.0);
}

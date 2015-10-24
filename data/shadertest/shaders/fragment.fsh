#version 300 es
precision highp float;

uniform sampler2D colortex;
uniform float gamma;

in vec2 tex;
in float depthval;

out vec4 OutColor;

void main ()
{
	OutColor = vec4(texture(colortex,tex).xyz/gamma,1.0);
}

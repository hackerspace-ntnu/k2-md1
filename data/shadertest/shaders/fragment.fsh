#version 300 es
precision highp float;

uniform sampler2D colortex;
uniform float gamma;

in vec2 tex;
in float depthval;

out vec4 OutColor;

void main ()
{
	float correction = length(vec2(0.5)-tex);
	vec2 translated = tex * correction;
	OutColor = vec4(texture(colortex,translated).zyx/gamma,1.0);
}

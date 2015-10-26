#version 300 es
precision highp float;

uniform sampler2D colortex;
uniform float gamma;
uniform float scaling;

in vec2 tex;
in float depthval;

out vec4 OutColor;

void main ()
{
        vec2 translated = vec2(0.5)+(tex-vec2(0.5))*scaling;
//        vec2 translated = tex;
        OutColor = vec4(texture(colortex,translated).zyx/gamma,1.0);
}

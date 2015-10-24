#version 300 es
precision highp float;

uniform mat4 transform;
uniform sampler2D depthtex;

layout(location = 0) in vec2 coord;
layout(location = 1) in vec2 texcoord;

out float depthval;
out vec2 tex;

void main(){
	float depth = texture(depthtex,texcoord);
	
	depthval = 1.0-depth;
	tex = texcoord;
	gl_Position = transform*vec4(vec3(coord,depth*5.0),1);
}

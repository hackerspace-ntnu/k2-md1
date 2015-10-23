#version 300 es
precision highp float;

uniform mat4 transform;
uniform sampler2D tex;

layout(location = 0) in vec2 coord;
layout(location = 1) in vec2 texcoord;

void main(){
	gl_Position = vec4(vec3(coord,0),1);
}

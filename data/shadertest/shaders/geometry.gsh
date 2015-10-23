#version 300 es
precision highp float;

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;

uniform mat4 transform;
uniform vec3 camera_pos;

out vec4 pos;
out vec2 tex;

void main(){
	float particleSize = 10.0;
	vec3 Pos = gl_in[0].gl_Position.xyz;
    vec3 toCamera = normalize(cameraPos - Pos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(toCamera, up) * particleSize;

    Pos -= right;
    gl_Position = transform * vec4(Pos, 1.0);
    texCoord = vec2(0.0, 0.0);
    position = gl_Position;
    EmitVertex();

    Pos.y += particleSize;
    gl_Position = transform * vec4(Pos, 1.0);
    texCoord = vec2(0.0, 1.0);
    position = gl_Position;
    EmitVertex();

    Pos.y -= particleSize;
    Pos += right;
    gl_Position = transform * vec4(Pos, 1.0);
    texCoord = vec2(1.0, 0.0);
    position = gl_Position;
    EmitVertex();

    Pos.y += particleSize;
    gl_Position = transform * vec4(Pos, 1.0);
    texCoord = vec2(1.0, 1.0);
    position = gl_Position;
    EmitVertex();

    EndPrimitive();
}

#version 330 core
layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Color;
layout(location = 2) in float Size;

out vec3 vColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vColor = Color;
    gl_Position = projection * view * vec4(Pos, 1.0);
    gl_PointSize = Size;
}
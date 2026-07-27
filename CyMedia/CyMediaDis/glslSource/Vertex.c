#version 330 core
layout(location = 0) in vec4 apos;
layout(location = 1) in vec2 atexcoord;
uniform mat4 m_matrix;
out vec2 TexCoord;
void main() {
    gl_Position = m_matrix * apos;
    TexCoord = atexcoord;
};
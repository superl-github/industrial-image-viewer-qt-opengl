#version 330 core
in vec4 apos;
in vec2 atexcoord;
uniform mat4 m_matrix;
out vec2 TexCoord;
void main() {
    gl_Position = m_matrix * apos;
    TexCoord = atexcoord;
};
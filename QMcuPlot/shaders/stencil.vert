#version 450

void main() {
    gl_Position = vec4(gl_VertexIndex, gl_VertexIndex, 0.0, 1.0);
}

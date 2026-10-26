#version 450

layout(location = 0) out vec2 outUV;

void main() {
    // Fullscreen triangle from vertex index
    float x = float((gl_VertexIndex << 1) & 2);
    float y = float(gl_VertexIndex & 2);
    vec2 uv = vec2(x, y);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    outUV = uv;
}
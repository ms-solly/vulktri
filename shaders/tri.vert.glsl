#version 450

vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5), // Left-bottom
    vec2(0.5, -0.5),  // Right-bottom
    vec2(-0.5, 0.5),  // Left-top
    vec2(0.5, 0.5)    // Right-top
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}

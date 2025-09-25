#version 450
layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) in vec2 inPos;
layout(location = 0) out vec3 fragColor;
vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5), // Left-bottom
    vec2(0.5, -0.5),  // Right-bottom
    vec2(-0.5, 0.5),  // Left-top
    vec2(0.5, 0.5)    // Right-top
);

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 0.0, 1.0);

    //gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}

#version 450
layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) out vec3 fragColor;
vec2 positions[3] = vec2[](
    vec2(400.0, 100.0),
    vec2(700.0, 500.0),
    vec2(100.0, 500.0)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}

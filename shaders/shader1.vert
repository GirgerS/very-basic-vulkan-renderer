#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


void main() {
    gl_Position = ubo.view * ubo.model * ubo.proj * vec4(inPosition, 1.0);
    fragColor = inColor;
}

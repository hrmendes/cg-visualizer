#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(push_constant) uniform Push { mat4 proj; } push;
layout(location = 0) out vec4 fragColor;
void main() {
    gl_Position = push.proj * vec4(inPosition, 0.0, 1.0);
    gl_PointSize = 6.0; 
    fragColor = inColor;
}
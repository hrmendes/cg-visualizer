#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(binding = 0) uniform sampler2D fontTexture;

layout(location = 0) out vec4 outColor;

void main() {
    // Amostra apenas o canal Red (R8_UNORM) para definir a transparência da letra
    float alpha = texture(fontTexture, fragUV).r;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
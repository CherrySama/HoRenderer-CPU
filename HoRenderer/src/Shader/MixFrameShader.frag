#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform uint frameCounter;
uniform sampler2D lastFrame;
uniform sampler2D nowFrame;

void main() {
    vec3 lastColor = texture(lastFrame, TexCoord).rgb;
    vec3 nowColor = texture(nowFrame, TexCoord).rgb;
    float blend_factor = 1.0f / float(frameCounter + 1.0f);
    vec3 color = mix(lastColor, nowColor, blend_factor);
    
    FragColor = vec4(color, 1.0f);
}
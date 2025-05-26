#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texPass0;

void main() {
    vec3 color = texture(texPass0, TexCoord).rgb;
    FragColor = vec4(color, 1.0f);
}
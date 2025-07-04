#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform uint frameCounter;
uniform sampler2D lastFrame;
uniform sampler2D nowFrame;

void main() {
    vec4 lastColor = texture(lastFrame, TexCoord);
    vec4 nowColor = texture(nowFrame, TexCoord);
    // if (frameCounter == 0u) {
    //     FragColor = nowColor;
    // } else {
    float weight = float(frameCounter) / float(frameCounter + 1u);
    FragColor = lastColor * weight + nowColor * (1.0 - weight);
    // }
}
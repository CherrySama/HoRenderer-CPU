#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texPass0;

void main()
{
   FragColor = texture(texPass0, TexCoord);
}
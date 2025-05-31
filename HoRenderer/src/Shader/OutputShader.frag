#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texPass0;

void main()
{
   vec4 color = texture(texPass0, TexCoord);
   // color.rgb = pow(color.rgb, vec3(1.0/2.2));
   FragColor = color;
}
#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texPass0;

vec3 ACESFilmicToneMapping(vec3 color)
{
   const float a = 2.51;
   const float b = 0.03;
   const float c = 2.43;
   const float d = 0.59;
   const float e = 0.14;
   return clamp((color * (a * color + b)) /
                (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 LinearToSRGB(vec3 linear)
{
   vec3 low = 12.92 * linear;
   vec3 high = 1.055 * pow(max(linear, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
   return mix(high, low, lessThanEqual(linear, vec3(0.0031308)));
}

void main()
{
   vec3 linear_radiance = max(texture(texPass0, TexCoord).rgb, vec3(0.0));
   vec3 display_color = LinearToSRGB(ACESFilmicToneMapping(linear_radiance));
   FragColor = vec4(display_color, 1.0);
}

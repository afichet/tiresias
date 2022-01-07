#version 330 core

layout(location = 0) out vec4 outColor;

in vec2 uv;

uniform sampler2D rgbImage;

uniform float exposure;
uniform bool sRGBGamma;
uniform vec3 gamma;

float to_sRGB_c(in float c) {
    if (abs(c) < 0.0031308) {
        return c*12.92;
    } else {
     	return 1.055*pow(c, 1.0/2.4) - 0.055;        
    }    
}

vec3 to_sRGB(in vec3 color) {
    return vec3(to_sRGB_c(color.r), to_sRGB_c(color.g), to_sRGB_c(color.b));
}

void main()
{
    outColor = texture(rgbImage, uv);

    outColor.rgb *= pow(2.f, exposure);

    if (sRGBGamma) {
        outColor.rgb = to_sRGB(outColor.rgb);
    } else {
        outColor.rgb = pow(outColor.rgb, 1.f/gamma);
    }
}
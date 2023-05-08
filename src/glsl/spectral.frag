#version 330 core

layout(location = 0) out vec4 outColor;

in vec2 uv;

// CMFs
uniform sampler1D cmfXYZ;
uniform uint cmfFirstWavelength;
uniform uint cmfSize;

// Illuminant
uniform sampler1D illuminant;
uniform uint illuminantFirstWavelength;
uniform uint illuminantSize;

uniform mat3 xyzToRgb;

// Spectral Image
uniform sampler3D spectralImage;
uniform sampler1D imageWavelengths;
uniform sampler1D imageWlBoundsWidths;

uniform int width;
uniform int height;
uniform uint nSpectralBands;
uniform bool isReflective;

void main()
{
    outColor = vec4(0., 0., 0., 1.);

    outColor.rgb = texture(spectralImage, vec3(0.5, uv)).rrr;

    outColor.rgb = xyzToRgb * texture(cmfXYZ, uv.x).xyz;

    ivec2 pxCoords = ivec2(
        int(round(uv.x * width)),
        int(round(uv.y * height))
    );

    vec3 pxColor = vec3(0);

    if (!isReflective) {
        for (int i = 0; i < int(nSpectralBands); i++) {
            float wl_curr  = texelFetch(imageWavelengths, i, 0).r;
            float wl_width = texelFetch(imageWlBoundsWidths, i, 0).r;

            float idx_img = float(i)/float(nSpectralBands-uint(1));

            float radiance = texture(spectralImage, vec3(idx_img, uv)).r;

            float idx_cmf = (wl_curr - float(cmfFirstWavelength)) / float(cmfSize - uint(1));

            vec3 cmfValue = texture(cmfXYZ, idx_cmf).xyz;

            pxColor += radiance * cmfValue * wl_width;
        }
    } else {
        float normalization_factor = 0;

        for (int i = 0; i < int(nSpectralBands); i++) {
            float wl_curr  = texelFetch(imageWavelengths, i, 0).r;
            float wl_width = texelFetch(imageWlBoundsWidths, i, 0).r;

            float idx_img = float(i)/float(nSpectralBands-uint(1));
            float radiance = texture(spectralImage, vec3(idx_img, uv)).r;

            for (int j = 0; j < wl_width; j++) {
                float idx_cmf = (wl_curr + j - float(cmfFirstWavelength)) / float(cmfSize - uint(1));
                float idx_illu = (wl_curr + j - float(illuminantFirstWavelength)) / float(illuminantSize - uint(1));

                vec3 cmfValue   = texture(cmfXYZ, idx_cmf).xyz;
                float illuValue = texture(illuminant, idx_illu).r;

                pxColor += radiance * illuValue * cmfValue;

                normalization_factor += illuValue * cmfValue.y;
            }
        }

        pxColor = pxColor.xyz / normalization_factor;
    }


    outColor.rgb = xyzToRgb * pxColor;
}
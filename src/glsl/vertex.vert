#version 330 core

layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 texCoords;

out vec2 uv;

uniform mat3 zoomMatrix;
uniform mat3 aspectMatrix;
uniform mat3 translateMatrix;

void main()
{
    vec3 vert = aspectMatrix * translateMatrix * zoomMatrix * vec3(vertPos, 1.);

    gl_Position = vec4(vert.xy / vert.z, 0., 1.);
    uv = texCoords;
}
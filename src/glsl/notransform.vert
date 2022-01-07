#version 330 core

layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 texCoords;

out vec2 uv;

void main()
{
    vec3 vert = vec3(vertPos, 1.);

    gl_Position = vec4(vert.xy / vert.z, 0., 1.);
    uv = texCoords;
}
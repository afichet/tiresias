#include "Shader.h"

#include <vector>
#include <exception>
#include <sstream>

#include <iostream>

Shader::Shader(
    const std::string &pathVertexShader,
    const std::string &pathFragmentShader)
{
    GLint status;

    std::vector<char> vert_src;
    std::vector<char> frag_src;

    // Vertex shader
    loadShaderSrc(pathVertexShader, vert_src);

    GLchar *vertex_sources[1] = {vert_src.data()};

    _vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(_vertexShader, 1, vertex_sources, NULL);
    glCompileShader(_vertexShader);
    glGetShaderiv(_vertexShader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        char log_buffer[512] = {0};
        glGetShaderInfoLog(_vertexShader, 512, NULL, log_buffer);

        std::stringstream ss;
        ss << "GLSL vertex shader " << pathVertexShader
           << " compilation failed:" << std::endl
           << log_buffer << std::endl;

        throw std::runtime_error(ss.str());
    }

    // Fragment shader
    loadShaderSrc(pathFragmentShader, frag_src);

    GLchar *fragment_sources[1] = {frag_src.data()};

    _fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(_fragmentShader, 1, fragment_sources, NULL);
    glCompileShader(_fragmentShader);
    glGetShaderiv(_fragmentShader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        char log_buffer[512] = {0};
        glGetShaderInfoLog(_fragmentShader, 512, NULL, log_buffer);

        std::stringstream ss;
        ss << "GLSL fragment shader " << pathFragmentShader
           << " compilation failed:" << std::endl
           << log_buffer << std::endl;

        throw std::runtime_error(ss.str());
    }

    // Link shaders
    _shaderProgram = glCreateProgram();
    glAttachShader(_shaderProgram, _vertexShader);
    glAttachShader(_shaderProgram, _fragmentShader);

    glLinkProgram(_shaderProgram);
}

Shader::~Shader()
{
    glDeleteShader(_shaderProgram);

    glDeleteProgram(_vertexShader);
    glDeleteProgram(_fragmentShader);
}

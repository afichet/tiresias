#pragma once

#include <string>
#include <fstream>
#include <vector>

#include <GL/glew.h>

class Shader
{
  public:
    Shader(
        const std::string &pathVertexShader,
        const std::string &pathFragmentShader);

    virtual ~Shader();

    GLuint get() const { return _shaderProgram; };

    virtual void gui() {}

    static void loadShaderSrc(
        const std::string &filename,
        std::vector<char> &source)
    {
        std::ifstream f_src(filename);

        f_src.seekg(0, std::ios::end);
        size_t file_size_byte = f_src.tellg();
        f_src.seekg(0, std::ios::beg);

        source.resize(file_size_byte + 1);
        f_src.read(source.data(), file_size_byte);
        source[file_size_byte] = 0;
    }

  protected:
    GLuint _vertexShader, _fragmentShader;
    GLuint _shaderProgram;
};
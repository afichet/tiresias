#include "ImageViewerSpectral.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <spectrum_data.h>


ImageViewerSpectral::ImageViewerSpectral()
    : ImageViewer()
    , _spectralNeedsUpdate(true)
{
    // Default
    // clang-format off
    _xyzToRgb = glm::mat3(
         3.2406, -0.9689,  0.0557,
        -1.5372,  1.8758, -0.2040,
        -0.4986,  0.0415,  1.0570);
    // clang-format on
}


ImageViewerSpectral::~ImageViewerSpectral()
{
    glDeleteFramebuffers(1, &_fbo_imageViewerSpectral);
}


// ----------------------------------------------------------------------------
// GUI elements
// ----------------------------------------------------------------------------

void ImageViewerSpectral::gui_inspectorTool()
{
    ImageViewer::gui_inspectorTool();

    ImGui::Separator();

    ImGui::Text("Spectral bands: %d", _nSpectralBands);
    ImGui::Text("Emissive: %s", _hasEmissive ? "Yes" : "No");
    ImGui::Text("Reflective: %s", _hasReflective ? "Yes" : "No");

    if (_hasEmissive) {
        ImGui::Text("Polarised: %s", _isPolarised ? "Yes" : "No");
    }
}


// ----------------------------------------------------------------------------
// OpenGL
// ----------------------------------------------------------------------------

void ImageViewerSpectral::initGL()
{
    ImageViewer::initGL();

    // ------------------------------------------------------------------------
    // Shader management
    // ------------------------------------------------------------------------

    _shaderProgram = std::unique_ptr<Shader>(new Shader("glsl/notransform.vert", "glsl/spectral.frag"));

    GLuint shaderId    = _shaderProgram->get();
    _loc_spectralImage = glGetUniformLocation(shaderId, "spectralImage");

    _loc_imageWavelengths    = glGetUniformLocation(shaderId, "imageWavelengths");
    _loc_imageWlBoundsWidths = glGetUniformLocation(shaderId, "imageWlBoundsWidths");

    _loc_width          = glGetUniformLocation(shaderId, "width");
    _loc_height         = glGetUniformLocation(shaderId, "height");
    _loc_nSpectralBands = glGetUniformLocation(shaderId, "nSpectralBands");
    _loc_isReflective   = glGetUniformLocation(shaderId, "isReflective");

    _loc_cmfXYZ             = glGetUniformLocation(shaderId, "cmfXYZ");
    _loc_cmfFirstWavelength = glGetUniformLocation(shaderId, "cmfFirstWavelength");
    _loc_cmfSize            = glGetUniformLocation(shaderId, "cmfSize");

    _loc_illuminant                 = glGetUniformLocation(shaderId, "illuminant");
    _loc_illuminantFirstWavelength  = glGetUniformLocation(shaderId, "illuminantFirstWavelength");
    _loc_illuminantSize             = glGetUniformLocation(shaderId, "illuminantSize");

    _loc_xyzToRgb = glGetUniformLocation(shaderId, "xyzToRgb");

    // ------------------------------------------------------------------------
    // Texture management
    // ------------------------------------------------------------------------

    glEnable(GL_TEXTURE_3D);
    glEnable(GL_TEXTURE_1D);

    // Managed later by subclasses that implements this class
    glGenTextures(1, &_tex_imageViewerSpectralIn);

    // Wavelengths
    glGenTextures(1, &_tex_imageWavelengths);
    glBindTexture(GL_TEXTURE_1D, _tex_imageWavelengths);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        _nSpectralBands,
        0,
        GL_RED,
        GL_FLOAT,
        _imageWavelengths.data());

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Wavelength bounds
    glGenTextures(1, &_tex_imageWlBoundsWidths);
    glBindTexture(GL_TEXTURE_1D, _tex_imageWlBoundsWidths);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        _nSpectralBands,
        0,
        GL_RED,
        GL_FLOAT,
        _imageWlBoundsWidths.data());

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // CMFs
    _cmfSize            = sizeof(SEXR::CIE1931_2DEG_X) / sizeof(SEXR::CIE1931_2DEG_X[0]);
    _cmfFirstWavelength = (unsigned int)SEXR::CIE1931_2DEG_FIRST_WAVELENGTH_NM;

    std::vector<float> cmfValues(3 * _cmfSize);
    for (size_t i = 0; i < _cmfSize; i++) {
        cmfValues[3 * i + 0] = SEXR::CIE1931_2DEG_X[i];
        cmfValues[3 * i + 1] = SEXR::CIE1931_2DEG_Y[i];
        cmfValues[3 * i + 2] = SEXR::CIE1931_2DEG_Z[i];
    }

    glGenTextures(1, &_tex_cmfXYZ);
    glBindTexture(GL_TEXTURE_1D, _tex_cmfXYZ);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGB32F,
        _cmfSize,
        0,
        GL_RGB,
        GL_FLOAT,
        cmfValues.data());

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_1D, 0);

    // Illuminant
    _illuminantSize = sizeof(SEXR::D_65_SPD) / sizeof(SEXR::D_65_SPD[0]);
    _illuminantFirstWavelength = (unsigned int)SEXR::D_65_FIRST_WAVELENGTH_NM;

    std::vector<float> illuminantValues(_illuminantSize);
    for (size_t i = 0; i < _illuminantSize; i++) {
        illuminantValues[i] = SEXR::D_65_SPD[i];
    }

    glGenTextures(1, &_tex_illuminant);
    glBindTexture(GL_TEXTURE_1D, _tex_illuminant);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        _illuminantSize,
        0,
        GL_RED,
        GL_FLOAT,
        illuminantValues.data());

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_1D, 0);

    // ------------------------------------------------------------------------
    // Main FBO
    // ------------------------------------------------------------------------

    glBindTexture(GL_TEXTURE_2D, _imageViewerInTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        imageWidth(),
        imageHeight(),
        0,
        GL_RGBA,
        GL_FLOAT,
        0);

    glGenFramebuffers(1, &_fbo_imageViewerSpectral);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo_imageViewerSpectral);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _imageViewerInTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


void ImageViewerSpectral::render()
{
    if (_spectralNeedsUpdate) {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo_imageViewerSpectral);
        glViewport(0, 0, imageWidth(), imageHeight());
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, _tex_imageViewerSpectralIn);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, _tex_imageWavelengths);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, _tex_imageWlBoundsWidths);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D, _tex_cmfXYZ);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_1D, _tex_illuminant);

        glUseProgram(_shaderProgram->get());

        // Set texture units
        glUniform1i(_loc_spectralImage, 0);
        glUniform1i(_loc_imageWavelengths, 1);
        glUniform1i(_loc_imageWlBoundsWidths, 2);
        glUniform1i(_loc_cmfXYZ, 3);
        glUniform1i(_loc_illuminant, 4);

        // Other parameters
        glUniform1ui(_loc_cmfFirstWavelength, _cmfFirstWavelength);
        glUniform1ui(_loc_cmfSize, _cmfSize);

        glUniform1ui(_loc_illuminantFirstWavelength, _illuminantFirstWavelength);
        glUniform1ui(_loc_illuminantSize, _illuminantSize);

        glUniformMatrix3fv(_loc_xyzToRgb, 1, GL_FALSE, glm::value_ptr(_xyzToRgb));

        glUniform1i(_loc_width, imageWidth());
        glUniform1i(_loc_height, imageHeight());
        glUniform1ui(_loc_nSpectralBands, _nSpectralBands);
        glUniform1i(_loc_isReflective, _hasReflective ? 1 : 0);

        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindTexture(GL_TEXTURE_3D, 0);
        glBindVertexArray(0);

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        _spectralNeedsUpdate = false;
    }

    ImageViewer::render();
}
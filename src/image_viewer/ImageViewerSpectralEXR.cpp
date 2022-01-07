#include "ImageViewerSpectralEXR.h"

#include <imgui.h>


ImageViewerSpectralEXR::ImageViewerSpectralEXR(
    const std::string &filepath)
    : ImageViewerSpectral()
    , _spectralImage(filepath)
{
    resizeImage(_spectralImage.width(), _spectralImage.height());
    _nSpectralBands = _spectralImage.nSpectralBands();
    _isPolarised    = _spectralImage.isPolarised();
    _hasEmissive    = _spectralImage.isEmissive();
    _hasReflective  = _spectralImage.isReflective();

    _imageWavelengths.resize(_nSpectralBands);

    for (size_t i = 0; i < _nSpectralBands; i++) {
        _imageWavelengths[i] = _spectralImage.wavelength_nm(i);
    }

    // TODO: support filtering / channel
    // Bounds
    _imageWlBoundsWidths.resize(_nSpectralBands);

    for (size_t i = 1; i < _nSpectralBands - 1; i++) {
        _imageWlBoundsWidths[i] = (_imageWavelengths[i + 1] - _imageWavelengths[i - 1]) / 2.f;
    }

    _imageWlBoundsWidths[0]                   = (_imageWavelengths[1] - _imageWavelengths[0]);
    _imageWlBoundsWidths[_nSpectralBands - 1] = (_imageWavelengths[_nSpectralBands - 1] - _imageWavelengths[_nSpectralBands - 2]);
}


void ImageViewerSpectralEXR::gui_inspectorTool()
{
    ImGui::Text("File format: Spectral EXR");
    ImageViewerSpectral::gui_inspectorTool();
}

// ----------------------------------------------------------------------------
// OpenGL
// ----------------------------------------------------------------------------

void ImageViewerSpectralEXR::initGL()
{
    ImageViewerSpectral::initGL();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, _tex_imageViewerSpectralIn);

    // The organization of the spectral layer is as follows:
    // for image of dimensions width, height and n_bands, the corresponding
    // memory location for x, y, band is at:
    // _spectralImage.emissive(0, 0, 0, 0)[width * h * band + width * y + x]

    if (_spectralImage.isEmissive()) {
        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            GL_R32F,
            _spectralImage.nSpectralBands(),
            _spectralImage.width(),
            _spectralImage.height(),
            0,
            GL_RED,
            GL_FLOAT,
            &_spectralImage.emissive(0, 0, 0, 0));
    } else {
        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            GL_R32F,
            _spectralImage.nSpectralBands(),
            _spectralImage.width(),
            _spectralImage.height(),
            0,
            GL_RED,
            GL_FLOAT,
            &_spectralImage.reflective(0, 0, 0));
    }

    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(GL_TEXTURE_3D, 0);
}
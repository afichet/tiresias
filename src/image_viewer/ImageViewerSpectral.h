#pragma once

#include "ImageViewer.h"

#include <string>
#include <vector>


class ImageViewerSpectral: public ImageViewer
{
  public:
    ImageViewerSpectral();

    virtual ~ImageViewerSpectral();

    // ------------------------------------------------------------------------
    // GUI elements
    // ------------------------------------------------------------------------

    virtual void gui_inspectorTool();

    // ------------------------------------------------------------------------
    // OpenGL
    // ------------------------------------------------------------------------

    virtual void initGL();
    virtual void render();

  protected:
    GLuint _tex_imageViewerSpectralIn;

    unsigned int       _nSpectralBands;
    std::vector<float> _imageWavelengths;
    // WARN: Subject to change
    std::vector<float> _imageWlBoundsWidths;


    bool _isPolarised;
    bool _hasEmissive;
    bool _hasReflective;

  private:
    std::unique_ptr<Shader> _shaderProgram;
    GLuint  _fbo_imageViewerSpectral;

    // Spectral conversion
    // GLuint _cmfXYZ[3];
    GLuint _tex_cmfXYZ;
    GLuint _tex_imageWavelengths;
    GLuint _tex_imageWlBoundsWidths;

    // Shader locations
    GLuint _loc_spectralImage;

    GLuint _loc_imageWavelengths;
    GLuint _loc_imageWlBoundsWidths;

    GLuint _loc_width, _loc_height, _loc_nSpectralBands;

    GLuint _loc_cmfXYZ;
    GLuint _loc_cmfFirstWavelength;
    GLuint _loc_cmfSize;

    GLuint _loc_xyzToRgb;

    unsigned int _cmfFirstWavelength;
    size_t       _cmfSize;

    glm::mat3 _xyzToRgb;

    bool _spectralNeedsUpdate;
};
#pragma once

#include "ImageViewerSpectral.h"

#include <EXRSpectralImage.h>

#include <string>


class ImageViewerSpectralEXR: public ImageViewerSpectral
{
  public:
    ImageViewerSpectralEXR(const std::string &filepath);

    virtual void gui_inspectorTool();

    virtual void initGL();

  private:
    SEXR::EXRSpectralImage _spectralImage;
};
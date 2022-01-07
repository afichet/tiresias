#pragma once

#include "ImageViewer.h"

#include <string>
#include <vector>


class ImageViewerLDR: public ImageViewer
{
  public:
    ImageViewerLDR(const std::string &filepath);

    virtual void initGL();

    virtual void gui_inspectorTool();

  protected:
    std::vector<unsigned char> _imageData;
};
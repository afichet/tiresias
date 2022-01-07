#include "ImageViewerLDR.h"

#include <exception>

#include <imgui/imgui.h>

#include <lodepng.h>


ImageViewerLDR::ImageViewerLDR(const std::string &filepath)
    : ImageViewer()
{
    unsigned int width, height;

    unsigned error = lodepng::decode(_imageData, width, height, filepath);

    if (error) {
        throw std::runtime_error(lodepng_error_text(error));
    }

    resizeImage(width, height);
}


void ImageViewerLDR::initGL()
{
    ImageViewer::initGL();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _imageViewerInTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_SRGB_ALPHA,
        imageWidth(),
        imageHeight(),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        _imageData.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}


void ImageViewerLDR::gui_inspectorTool()
{
    ImGui::Text("File format: PNG");
    ImageViewer::gui_inspectorTool();
}
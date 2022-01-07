#pragma once

#include <Shader.h>

#include <GL/glew.h>

#include <glm/glm.hpp>


class ImageViewer
{
  public:
    ImageViewer();

    virtual ~ImageViewer();

    // ------------------------------------------------------------------------
    // GUI elements
    // ------------------------------------------------------------------------

    virtual void gui();

    // Controls
    virtual void gui_colorControl();
    virtual void gui_viewControl();

    // Tools
    virtual void gui_inspectorTool();

    // Menu
    virtual void menuImageControls();
    virtual void menuImageTools();

    // ------------------------------------------------------------------------
    // OpenGL
    // ------------------------------------------------------------------------

    virtual void initGL();

    virtual void render();

    virtual GLuint getTexture() const;

    virtual void resizeWindow(unsigned int width, unsigned int height);
    virtual void resizeImage(unsigned int width, unsigned int height);
    virtual void updateAspect();

    virtual void mouseOver(int xpos, int ypos);
    virtual void mouseScroll(double xoffset, double yoffset);

    virtual void mouseLeftPress(double xpos, double ypos);
    virtual void mouseLeftDrag(double xpos, double ypos);
    virtual void mouseLeftRelease(double xpos, double ypos);

    unsigned int imageWidth() const { return _imageWidth; }
    unsigned int imageHeight() const { return _imageHeight; }

    // ------------------------------------------------------------------------
    // Utility functions
    // ------------------------------------------------------------------------

    float getAbsoluteZoom() const
    {
        return _windowWidth * _zoomMatrix[0].x / _imageWidth;
    }

    float getRelativeZoom() const
    {
        return _zoomMatrix[0].x;
    }

    void setAbsoluteZoom(float zoom);

    void setFitView();

    glm::vec2 windowToImage(const glm::vec2 &windowCoords) const;
    glm::vec2 imageToWindow(const glm::vec2 &imageCoords) const;

  protected:
    GLuint _vbo, _ebo, _vao;
    GLuint _imageViewerInTexture;

    // GUI
    bool _showViewControl;
    bool _showColorControl;
    bool _showInspector;

    // Mouse control
    int   _mouseOverX, _mouseOverY;
    float _startDragX, _startDragY;

    // Image tranformation
    float _zoom;

    glm::mat3 _zoomMatrix;
    glm::mat3 _aspectMatrix;
    glm::mat3 _translateMatrix;
    glm::mat3 _prevTranslateMatrix;

    // Color control
    float     _exposure;
    bool      _sRGBGamma;
    bool      _grayGamma;
    glm::vec3 _gamma;

    float _colorAtMousePosition[4];
    int   _xImageMouseOver, _yImageMouseOver;

  private:
    Shader *_shaderProgram;

    // Image transformation
    GLuint _loc_zoomMatrix;
    GLuint _loc_aspectMatrix;
    GLuint _loc_translateMatrix;

    // Color control
    GLuint _loc_exposure;
    GLuint _loc_sRGBGamma;
    GLuint _loc_gamma;

    unsigned int _imageWidth, _imageHeight;
    unsigned int _windowWidth, _windowHeight;

    // Framebuffer
    GLuint _imageViewerFBO;
    GLuint _imageViewerOutTexture;

    // Internal use for GUI
    bool _windowSizeSet;
    bool _imageSizeSet;
};

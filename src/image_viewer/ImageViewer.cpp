#include "ImageViewer.h"

#include "Util.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>

#include <exception>
#include <vector>


ImageViewer::ImageViewer()
    : _vbo(0)
    , _ebo(0)
    , _vao(0)
    // GUI
    , _showViewControl(true)
    , _showColorControl(true)
    , _showInspector(true)
    // Image transformation
    , _zoom(1.f)
    , _zoomMatrix(glm::mat3(1.f))
    , _aspectMatrix(glm::mat3(1.f))
    , _translateMatrix(glm::mat3(1.f))
    // Color control
    , _exposure(0.f)
    , _sRGBGamma(true)
    , _grayGamma(true)
    , _gamma(2.2f)
    // Image display
    , _shaderProgram(nullptr)
    , _imageWidth(0)
    , _imageHeight(0)
    , _windowWidth(0)
    , _windowHeight(0)
    // Internal use for GUI
    , _windowSizeSet(false)
    , _imageSizeSet(false)
{
}


ImageViewer::~ImageViewer()
{
    glDeleteBuffers(1, &_ebo);
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    glDeleteTextures(1, &_imageViewerInTexture);

    glDeleteFramebuffers(1, &_imageViewerFBO);
    glDeleteTextures(1, &_imageViewerOutTexture);
}


// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------

void ImageViewer::gui()
{
    ImVec2 pos        = ImGui::GetWindowPos();
    ImVec2 windowSize = ImVec2(0., 0.);

    if (_showViewControl) {
        ImGui::Begin("View", &_showViewControl);
        gui_viewControl();
        windowSize = ImGui::GetWindowSize();
        ImGui::End();

        // Set Next window position and size
        pos.y += windowSize.y;
        ImGui::SetNextWindowPos(ImVec2(0, pos.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, 0.), ImGuiCond_FirstUseEver);
    }

    if (_showColorControl) {
        ImGui::Begin("Colors", &_showColorControl);
        gui_colorControl();
        windowSize = ImGui::GetWindowSize();
        ImGui::End();

        // Set Next window position and size
        pos.y += windowSize.y;
        ImGui::SetNextWindowPos(ImVec2(0, pos.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, 0.), ImGuiCond_FirstUseEver);
    }

    // FIXME: position is not set in subclasses
    if (_showInspector) {
        ImGui::Begin("Inspector", &_showInspector);
        gui_inspectorTool();
        windowSize = ImGui::GetWindowSize();
        ImGui::End();

        // Set Next window position and size
        pos.y += windowSize.y;
        ImGui::SetNextWindowPos(ImVec2(0, pos.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, 0.), ImGuiCond_FirstUseEver);
    }
}


void ImageViewer::gui_colorControl()
{
    ImGui::SliderFloat("Exposure", &_exposure, -10.f, 10.f);
    ImGui::Checkbox("sRGB", &_sRGBGamma);

    if (!_sRGBGamma) {
        ImGui::SameLine();
        ImGui::Checkbox("Gray gamma", &_grayGamma);

        if (_grayGamma) {
            ImGui::SliderFloat("Gamma", &_gamma.r, 0.1f, 5.f);
            _gamma.g = _gamma.r;
            _gamma.b = _gamma.r;
        } else {
            ImGui::SliderFloat3("Gamma R, G, B", glm::value_ptr(_gamma), 0.1f, 5.f);
        }
    }

    // float nextPosX = ImGui::GetWindowPos().x;
    // float nextPosY = ImGui::GetWindowPos().y + ImGui::GetWindowHeight();
}


void ImageViewer::gui_viewControl()
{
    float absoluteZoom = getAbsoluteZoom() * 100.f;

    ImGui::Text("Zoom");
    ImGui::SliderFloat("%", &absoluteZoom, 1, 200);

    if (_imageSizeSet && _windowSizeSet) {
        setAbsoluteZoom(absoluteZoom / 100.f);
    }

    if (ImGui::Button("100%")) {
        setAbsoluteZoom(1.f);
    };
    ImGui::SameLine();
    if (ImGui::Button("Fit view")) {
        setFitView();
    }
    ImGui::Separator();
}


void ImageViewer::gui_inspectorTool()
{
    ImGui::Text("Image Size: %dx%d", _imageWidth, _imageHeight);
    ImGui::Text("x: %d, y: %d", _mouseOverX, _mouseOverY);

    const glm::vec2 posImage  = windowToImage(glm::vec2(_mouseOverX, _mouseOverY));
    const int       posImageX = std::floor(posImage.x);
    const int       posImageY = std::floor(posImage.y);

    if (posImageX >= 0 && posImageX < _imageWidth
        && posImageY >= 0 && posImageY < _imageHeight) {
        ImGui::Text("x: %d, y: %d", posImageX, posImageY);
    }

    // ImGui::Text("x: %d, y: %d", _xImageMouseOver, _yImageMouseOver);

    // if (   _xImageMouseOver >= 0 && _xImageMouseOver < _imageWidth
    //     && _yImageMouseOver >= 0 && _yImageMouseOver < _imageHeight) {
    //     // Color
    //     ImGui::Text("R: %f, G: %f, B: %f", _colorAtMousePosition[0], _colorAtMousePosition[1], _colorAtMousePosition[2]);
    //     ImGui::InputFloat3("Raw Color", _colorAtMousePosition);
    // }
}


void ImageViewer::menuImageControls()
{
    ImGui::MenuItem("Colors", NULL, &_showColorControl);
}


void ImageViewer::menuImageTools()
{
    ImGui::MenuItem("Inspector", NULL, &_showInspector);
    ImGui::MenuItem("View", NULL, &_showViewControl);
}


// ----------------------------------------------------------------------------
// OpenGL
// ----------------------------------------------------------------------------

void ImageViewer::initGL()
{
    // ------------------------------------------------------------------------
    // Shader management
    // ------------------------------------------------------------------------

    _shaderProgram = std::unique_ptr<Shader>(new Shader("glsl/vertex.vert", "glsl/fragment.frag"));

    const GLuint shaderId = _shaderProgram->get();

    _loc_zoomMatrix      = glGetUniformLocation(shaderId, "zoomMatrix");
    _loc_aspectMatrix    = glGetUniformLocation(shaderId, "aspectMatrix");
    _loc_translateMatrix = glGetUniformLocation(shaderId, "translateMatrix");

    _loc_exposure  = glGetUniformLocation(shaderId, "exposure");
    _loc_sRGBGamma = glGetUniformLocation(shaderId, "sRGBGamma");
    _loc_gamma     = glGetUniformLocation(shaderId, "gamma");

    // ------------------------------------------------------------------------
    // Geometry management
    // ------------------------------------------------------------------------

    // clang-format off
    const float vertex_data[] = {
        // Coordinates   Texture
        -1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, 1.f, 1.f
    };

    // Element buffer object
    GLuint quad[] = {
        0, 1, 2,
        2, 3, 0
    };
    // clang-format on

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ------------------------------------------------------------------------
    // Texture management
    // ------------------------------------------------------------------------
    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &_imageViewerInTexture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _imageViewerInTexture);

    float borderColor[] = {0.f, 0.f, 0.f, 0.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    // ------------------------------------------------------------------------
    // Main FBO
    // ------------------------------------------------------------------------

    glGenTextures(1, &_imageViewerOutTexture);
    glBindTexture(GL_TEXTURE_2D, _imageViewerOutTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        _windowWidth,
        _windowHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &_imageViewerFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, _imageViewerFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _imageViewerOutTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


void ImageViewer::render()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _imageViewerFBO);
    glViewport(0, 0, _windowWidth, _windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_shaderProgram->get());

    glUniformMatrix3fv(_loc_zoomMatrix, 1, GL_FALSE, glm::value_ptr(_zoomMatrix));
    glUniformMatrix3fv(_loc_aspectMatrix, 1, GL_FALSE, glm::value_ptr(_aspectMatrix));
    glUniformMatrix3fv(_loc_translateMatrix, 1, GL_FALSE, glm::value_ptr(_translateMatrix));
    glUniform1f(_loc_exposure, _exposure);
    glUniform1i(_loc_sRGBGamma, _sRGBGamma);
    glUniform3fv(_loc_gamma, 1, glm::value_ptr(_gamma));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _imageViewerInTexture);
    glBindVertexArray(_vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    // // to NDC
    // glm::vec3 pos = glm::vec3(
    //     2.f * (float)_mouseOverX/(float)_windowWidth - 1.f,
    //     2.f * (float)_mouseOverY/(float)_windowHeight - 1.f,
    //     1.);

    // glm::mat3 transform = _aspectMatrix * _translateMatrix * _zoomMatrix;
    // glm::vec3 pos_s = glm::inverse(transform) * pos;

    // // NDC to 0..1
    // glm::vec2 coordsImage = glm::vec2(pos_s.x, pos_s.y)/pos_s.z;
    // coordsImage = (coordsImage + glm::vec2(1.f))/2.f;
    // coordsImage *= glm::vec2(_imageWidth, _imageHeight);

    // _xImageMouseOver = (int)std::round(coordsImage.x);
    // _yImageMouseOver = (int)std::round(coordsImage.y);

    // glReadPixels(_xImageMouseOver, _yImageMouseOver, 1, 1, GL_RGBA, GL_FLOAT, _colorAtMousePosition);


    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


GLuint ImageViewer::getTexture() const
{
    return _imageViewerOutTexture;
}


// ----------------------------------------------------------------------------
// Set aspect ratio
// ----------------------------------------------------------------------------

void ImageViewer::resizeWindow(unsigned int width, unsigned int height)
{
    _windowSizeSet = true;

    if (_windowWidth != width || _windowHeight != height) {
        _windowWidth  = width;
        _windowHeight = height;

        updateAspect();

        // Resize FBO's texture
        glBindTexture(GL_TEXTURE_2D, _imageViewerOutTexture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            _windowWidth,
            _windowHeight,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}


void ImageViewer::resizeImage(unsigned int width, unsigned int height)
{
    _imageSizeSet = true;

    _imageWidth  = width;
    _imageHeight = height;

    updateAspect();
}


void ImageViewer::updateAspect()
{
    const float aspectWindow = (float)_windowWidth / (float)_windowHeight;
    const float aspectImage  = (float)_imageWidth / (float)_imageHeight;

    const float aspect = aspectImage / aspectWindow;

    // clang-format off
    if (aspect > 1.) {
        _aspectMatrix = glm::mat3(
            1.f, 0.f, 0.f,
            0.f, 1.f/aspect, 0.f,
            0.f, 0.f, 1.f
        );
    } else {
        _aspectMatrix = glm::mat3(
            aspect, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f
        );
    }
    // clang-format on
}


// ----------------------------------------------------------------------------
// Mouse control
// ----------------------------------------------------------------------------

void ImageViewer::mouseOver(int xpos, int ypos)
{
    _mouseOverX = xpos;
    _mouseOverY = ypos;
}


void ImageViewer::mouseScroll(double xoffset, double yoffset)
{
    const float zoomFactor = std::exp(yoffset / 10.f);

    // clang-format off
    glm::mat3 zoomFactorMatrix(
        zoomFactor, 0.f, 0.f,
        0.f, zoomFactor, 0.f,
        0.f, 0.f, 1.f
    );
    // clang-format on

    // We want to zoom to the current central pixel
    zoomFactorMatrix = inverse(_translateMatrix) * zoomFactorMatrix * _translateMatrix;

    _zoomMatrix = zoomFactorMatrix * _zoomMatrix;
}


void ImageViewer::mouseLeftPress(double xpos, double ypos)
{
    _startDragX = xpos;
    _startDragY = ypos;

    _prevTranslateMatrix = _translateMatrix;
}


void ImageViewer::mouseLeftDrag(double xpos, double ypos)
{
    const float deltaX = xpos - _startDragX;
    const float deltaY = ypos - _startDragY;

    const float aspectWindow = (float)_windowWidth / (float)_windowHeight;
    const float aspectImage  = (float)_imageWidth / (float)_imageHeight;

    const float aspect = aspectImage / aspectWindow;

    float translateX, translateY;

    if (aspect > 1.) {
        translateX = 2.f * deltaX / _windowWidth;
        translateY = 2.f * deltaY / _windowHeight * aspect;
    } else {
        translateX = 2.f * deltaX / _windowWidth / aspect;
        translateY = 2.f * deltaY / _windowHeight;
    }

    // clang-format off
    _translateMatrix = glm::mat3(
        1.f, 0.f, 0.f, 
        0.f, 1.f, 0.f,
        translateX, translateY, 1.f) * _prevTranslateMatrix;
    // clang-format on
}


void ImageViewer::mouseLeftRelease(double xpos, double ypos)
{
    mouseLeftDrag(xpos, ypos);
}


void ImageViewer::setAbsoluteZoom(float zoom)
{
    if (zoom > 0. && !std::isnan(zoom) && !std::isinf(zoom)) {
        const float relZoom    = zoom * _imageWidth / _windowWidth;
        const float zoomFactor = relZoom / getRelativeZoom();

        // clang-format off
        glm::mat3 zoomFactorMatrix(
            zoomFactor, 0.f, 0.f,
            0.f, zoomFactor, 0.f,
            0.f, 0.f, 1.f
        );
        // clang-format on

        zoomFactorMatrix = inverse(_translateMatrix) * zoomFactorMatrix * _translateMatrix;

        _zoomMatrix = zoomFactorMatrix * _zoomMatrix;
    }
}


void ImageViewer::setFitView()
{
    _zoomMatrix      = glm::mat3(1.f);
    _translateMatrix = glm::mat3(1.f);
}


glm::vec2 ImageViewer::windowToImage(const glm::vec2 &windowCoords) const
{
    // Window coordinates to Normalized Device Coordiantes (NDC)
    const glm::vec2 ndc = 2.f * windowCoords / glm::vec2(_windowWidth - 1, _windowHeight - 1)
                          - glm::vec2(1.f);

    // Transformation chain applied in vertex shader
    const glm::mat3 transformation = _aspectMatrix * _translateMatrix * _zoomMatrix;

    // Vertex position from the NDC
    const glm::vec3 vertPos = inverse(transformation) * glm::vec3(ndc, 1.f);

    // Texture coordinates from vertex position
    const glm::vec3 uvCoords = (vertPos / vertPos.z + glm::vec3(1.f, 1.f, 0.f)) / 2.f;

    // Texture coordinates to image coordinates
    return glm::vec2(_imageWidth * uvCoords.x, _imageHeight * uvCoords.y);
}


glm::vec2 ImageViewer::imageToWindow(const glm::vec2 &imageCoords) const
{
    // Image coordinates to vertex coordinates
    const glm::vec2 vertPos = 2.f * imageCoords / glm::vec2(_imageWidth - 1, _imageHeight - 1)
                              - glm::vec2(1.f);

    // Transformation chain applied in vertex shader
    const glm::mat3 transformation = _aspectMatrix * _translateMatrix * _zoomMatrix;

    const glm::vec3 ndc = transformation * glm::vec3(vertPos, 1.f);

    // const glm::vec3 windowCoords = ndc
}
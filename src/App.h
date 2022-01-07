#pragma once

#include "image_viewer/ImageViewer.h"

#include <GLFW/glfw3.h>

#include <imgui_internal.h>

#include <mutex>

class App
{
  public:
    App(int argc, char *argv[]);

    virtual ~App();

    virtual void exec();

    virtual void open(const std::string &path);

  protected:
    virtual void initGL();

    virtual void menuBar();
    virtual void gui();

    virtual void mouseMove(double xpos, double ypos);
    virtual void mouseButton(int button, int action, int mods);

    virtual void keyPress(int key, int scancode, int mods);

    virtual void resize(int width, int height);

    // GLFW static callbacks
    static void glfw_error_cb(int error, const char *description);
    static void glfw_drop_cb(GLFWwindow *window, int count, const char **paths);
    static void glfw_cursorpos_cb(GLFWwindow *window, double xpos, double ypos);
    static void glfw_mousebutton_cb(GLFWwindow *window, int button, int action, int mods);
    static void glfw_key_cb(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfw_window_size_cb(GLFWwindow *window, int width, int height);

    GLFWwindow *_window;
    GLFWcursor *_cursorHand;

    ImageViewer *_imageViewer;
    std::mutex   _imageViewerMutex;

    bool _leftMouseButtonPressed;


    int _width, _height;

    bool _requestOpen;



    ImGuiID _dockSpaceID = 0;
};
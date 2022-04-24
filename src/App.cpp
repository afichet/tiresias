#include "App.h"

#include <exception>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_demo.cpp>

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "image_viewer/ImageViewerLDR.h"
#include "image_viewer/ImageViewerSpectralEXR.h"

#include <ImFileDialog.h>
#include <nfd.h>

App::App(int argc, char *argv[])
    // : _imageViewer(new ImageViewerLDR("image_w.png"))
    : _imageViewer(new ImageViewerSpectralEXR("MatProbe.exr"))
    , _leftMouseButtonPressed(false)
    , _requestOpen(false)
{
    // ------------------------------------------------------------------------
    // GLFW initialization
    // ------------------------------------------------------------------------

    if (!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }

    glfwSetErrorCallback(glfw_error_cb);

    const char *glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    _window = glfwCreateWindow(1280, 720, "Tiresias", NULL, NULL);
    glfwSetWindowUserPointer(_window, this);

    if (!_window) {
        throw std::runtime_error("Could not create a window");
    }

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);   // Enable vsync

    _cursorHand = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);

    // ------------------------------------------------------------------------
    // OpenGL initialization
    // ------------------------------------------------------------------------

    initGL();

    // ------------------------------------------------------------------------
    // Callbacks
    // ------------------------------------------------------------------------

    glfwSetDropCallback(_window, glfw_drop_cb);
    glfwSetCursorPosCallback(_window, glfw_cursorpos_cb);
    glfwSetMouseButtonCallback(_window, glfw_mousebutton_cb);
    glfwSetKeyCallback(_window, glfw_key_cb);
    glfwSetWindowSizeCallback(_window, glfw_window_size_cb);

    // ------------------------------------------------------------------------
    // ImGui initialization
    // ------------------------------------------------------------------------

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();


    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.FontAllowUserScaling              = true;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);



    // ImFileDialog requires you to set the CreateTexture and DeleteTexture
    ifd::FileDialog::Instance().CreateTexture = [](uint8_t *data, int w, int h, char fmt) -> void * {
        GLuint tex;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return (void *)tex;
    };
    ifd::FileDialog::Instance().DeleteTexture = [](void *tex) {
        GLuint texID = (GLuint)((uintptr_t)tex);
        glDeleteTextures(1, &texID);
    };
}


App::~App()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(_window);
    glfwDestroyCursor(_cursorHand);
    glfwTerminate();
}


void App::exec()
{
    bool _demo = true;
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        glViewport(0, 0, _width, _height);
        glClear(GL_COLOR_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui::ShowDemoWindow(&_demo);

        gui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(_window);
    }
}


void App::open(const std::string &path)
{
    _imageViewerMutex.lock();
    if (_imageViewer) {
        delete _imageViewer;
        _imageViewer = nullptr;
    }

    std::string ext = path.substr(path.find_last_of("."));
    std::cout << "extension = " << ext << std::endl;

    if (ext == ".exr") {
        _imageViewer = new ImageViewerSpectralEXR(path);
    } else if (ext == ".png") {
        _imageViewer = new ImageViewerLDR(path);
    }

    if (_imageViewer) {
        _imageViewer->initGL();
    }

    _imageViewerMutex.unlock();
}


void App::initGL()
{
    // Init GLEW
    glewExperimental = GL_TRUE;
    GLenum err_glew  = glewInit();

    if (err_glew != GLEW_OK) {
        std::cerr << "[ERROR] " << glewGetErrorString(err_glew) << std::endl;
    }

    // We do not want any clamping, potentially working with HDR framebuffers
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

    _imageViewerMutex.lock();
    if (_imageViewer) {
        _imageViewer->initGL();
    }
    _imageViewerMutex.unlock();
}


void App::menuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl + O")) {
                _requestOpen = true;
            }
            if (ImGui::MenuItem("Exit", "Alt + F4")) {
                glfwSetWindowShouldClose(_window, true);
            }
            ImGui::EndMenu();
        }

        _imageViewerMutex.lock();
        if (_imageViewer) {
            if (ImGui::BeginMenu("Controls")) {
                _imageViewer->menuImageControls();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                _imageViewer->menuImageTools();
                ImGui::EndMenu();
            }
        }
        _imageViewerMutex.unlock();

        ImGui::EndMainMenuBar();
    }
}


void App::gui()
{
    menuBar();


    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());


    // const auto viewport = ImGui::GetMainViewport();

    // // m_LastSize = viewport->Size;

    // ImGui::SetNextWindowPos(viewport->WorkPos);
    // ImGui::SetNextWindowSize(viewport->WorkSize);
    // ImGui::SetNextWindowViewport(viewport->ID);

    // auto host_window_flags = 0;
    // host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    // host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    // host_window_flags |= ImGuiWindowFlags_NoBackground;

    // char label[100 + 1];
    // ImFormatString(label, 100, "DockSpaceViewport_%08X", viewport->ID);

    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // const auto res = ImGui::Begin(label, nullptr, host_window_flags);
    // ImGui::PopStyleVar(3);

    // _dockSpaceID = ImGui::GetID("MyDockSpace");
    // if (ImGui::DockSpace(_dockSpaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode)) {
    //     ImGui::End();
    // }


    // ImGui::DockBuilderAddNode(_dockSpaceID, ImGuiDockNodeFlags_DockSpace);

    // float leftPaneDefaultWidth = 300;

    // auto dockMainID = _dockSpaceID; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
    // const auto dockLeftID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, leftPaneDefaultWidth, nullptr, &dockMainID);

    // ImGui::DockBuilderDockWindow("Debug", dockMainID);

    bool p_open = true;

    static bool               opt_fullscreen  = true;
    static bool               opt_padding     = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &p_open, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    _imageViewerMutex.lock();

    if (_imageViewer) {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 0.f), ImGuiCond_FirstUseEver);
        _imageViewer->gui();


        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(300, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Image");
        ImVec2 size = ImGui::GetContentRegionAvail();
        _imageViewer->resizeWindow(size.x, size.y);
        _imageViewer->render();

        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImGui::Image((void *)(intptr_t)_imageViewer->getTexture(), size);

        if (ImGui::IsItemHovered()) {
            const float rel_pos_x = io.MousePos.x - pos.x;
            const float rel_pos_y = io.MousePos.y - pos.y;

            _imageViewer->mouseOver(rel_pos_x, rel_pos_y);

            // Process click events
            if (io.MouseClicked[0]) {
                _imageViewer->mouseLeftPress(io.MousePos.x, io.MousePos.y);
                _leftMouseButtonPressed = true;
            }

            // Handling mouse move form GLFW:
            // we want to be able to click within the window and move even when
            // the cursor gets out of the boudaries of the frame.
            // This is the same behavior than this but taking into account mouse movement out
            // of the window.
            // if (_leftMouseButtonPressed) {
            //     _imageViewer->mouseLeftDrag(io.MousePos.x, io.MousePos.y);
            // }

            // if (io.MouseReleased[0]) {
            //     _leftMouseButtonPressed = false;
            //     _imageViewer->mouseLeftRelease(io.MousePos.x, io.MousePos.y);
            // }

            // Process scroll events
            if (io.MouseWheel != 0.0f) {
                _imageViewer->mouseScroll(0., io.MouseWheel);
            }
        }
        ImGui::End();
    }

    _imageViewerMutex.unlock();

    // This version uses the DearImGUI backend
    // if (_requestOpen) {
    //     ifd::FileDialog::Instance().Open("ImageOpenDialog", "Open", "Image  file (*.png;*.exr){.png,.exr}");

    //     if (ifd::FileDialog::Instance().IsDone("ImageOpenDialog")) {
    //         if (ifd::FileDialog::Instance().HasResult()) {
    //             const std::string res = ifd::FileDialog::Instance().GetResult().u8string();
    //             open(res);
    //         }
    //         ifd::FileDialog::Instance().Close();
    //         _requestOpen = false;
    //     }
    // }

    ImGui::End();


    if (_requestOpen) {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath );
        std::string res;

        switch (result) {
            case NFD_OKAY:
                res = outPath;
                open(res);
                break;

            case NFD_CANCEL:
                break;

            case NFD_ERROR:
                break;
        }

        free(outPath);

        _requestOpen = false;
    }
}


void App::mouseMove(double xpos, double ypos)
{
    _imageViewerMutex.lock();
    if (_leftMouseButtonPressed && _imageViewer) {
        _imageViewer->mouseLeftDrag(xpos, ypos);
    }
    _imageViewerMutex.unlock();
}


void App::mouseButton(int button, int action, int mods)
{
    if (_leftMouseButtonPressed) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            _leftMouseButtonPressed = false;

            _imageViewerMutex.lock();
            if (_imageViewer) {
                double xpos, ypos;
                glfwGetCursorPos(_window, &xpos, &ypos);
                _imageViewer->mouseLeftRelease(xpos, ypos);
            }
            _imageViewerMutex.unlock();
        }
    }
}


void App::keyPress(int key, int scancode, int mods)
{
    // Setup shortcuts
    if (mods == GLFW_MOD_CONTROL) {
        if (key == GLFW_KEY_O) {
            _requestOpen = true;
        }
    }
}


void App::resize(int width, int height)
{
    _width  = width;
    _height = height;
}


void App::glfw_error_cb(int error, const char *description)
{
    std::cerr << "[ERROR] GLFW error: "
              << error << ": "
              << description << std::endl;
}


void App::glfw_drop_cb(GLFWwindow *window, int count, const char **paths)
{
    App *p = (App *)glfwGetWindowUserPointer(window);
    if (count > 0) {
        std::string filepath = paths[0];
        p->open(filepath);
    }
}


void App::glfw_cursorpos_cb(GLFWwindow *window, double xpos, double ypos)
{
    App *p = (App *)glfwGetWindowUserPointer(window);

    p->mouseMove(xpos, ypos);
}


void App::glfw_mousebutton_cb(GLFWwindow *window, int button, int action, int mods)
{
    App *p = (App *)glfwGetWindowUserPointer(window);

    p->mouseButton(button, action, mods);
}


void App::glfw_key_cb(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    App *p = (App *)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        p->keyPress(key, scancode, mods);
    }
}


void App::glfw_window_size_cb(GLFWwindow *window, int width, int height)
{
    App *p = (App *)glfwGetWindowUserPointer(window);

    p->resize(width, height);
}

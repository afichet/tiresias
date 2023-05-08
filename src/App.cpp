#include "App.h"

#include <exception>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
// #include <imgui/imgui_demo.cpp>

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "image_viewer/ImageViewerLDR.h"
#include "image_viewer/ImageViewerSpectralEXR.h"

#include <nfd.h>


App::App(int argc, char *argv[])
    // : _imageViewer(new ImageViewerLDR("image_w.png"))
    : _imageViewer(nullptr)
    , _leftMouseButtonPressed(false)
    , _requestOpen(false)
    , _layoutInitialized(false)
{
    // ------------------------------------------------------------------------
    // GLFW initialization
    // ------------------------------------------------------------------------

    if (!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }

    glfwSetErrorCallback(glfw_error_cb);

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

    const char *glsl_version = "#version 330";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // // Default GUI font
    // io.Fonts->AddFontFromFileTTF(
    //     "assets/OpenSans-Regular.ttf",
    //     20,
    //     NULL,
    //     io.Fonts->GetGlyphRangesGreek()
    // );

    // // Icons font
    // ImFontConfig config;
    // config.MergeMode = true;
    // config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced

    // static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0 };

    // io.Fonts->AddFontFromFileTTF(
    //     "assets/" FONT_ICON_FILE_NAME_MD,
    //     14,
    //     &config, icon_ranges
    // );

    // // Monospaced font
    // _font_mono = io.Fonts->AddFontFromFileTTF(
    //     "assets/Hack-Regular.ttf",
    //     20, NULL, NULL
    // );

    ImGui::StyleColorsDark();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.FontAllowUserScaling              = true;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Check if an image was provided as an argument
    if (argc > 1) {
        open(argv[1]);
    }
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
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        // glViewport(0, 0, _width, _height);
        glClear(GL_COLOR_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

    std::string ext = path.substr(path.find_last_of("."));
    std::cout << "extension = " << ext << std::endl;

    std::shared_ptr<ImageViewer> new_image;

    // TODO: cleaner exception handling and support for RGB EXRs
    if (ext == ".exr" || ext == ".EXR") {
        try {
            new_image = std::shared_ptr<ImageViewerSpectralEXR>(new ImageViewerSpectralEXR(path));
        } catch (const SEXR::SpectralImage::Errors& e) {
            std::cout << "Error while opening \"" << path << "\": It is not a spectral image" << std::endl;
            new_image = nullptr;
        }
    } else if (ext == ".png" || ext == ".PNG") {
        new_image = std::shared_ptr<ImageViewerLDR>(new ImageViewerLDR(path));
    } else {
        std::cout << "Unsupported image type" << std::endl;
    }
        
    if (new_image) {
        _imageViewer = new_image;
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


void App::initializeLayout()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

    _dock_mainCentralId = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
    _dock_leftId = ImGui::DockBuilderSplitNode(_dock_mainCentralId, ImGuiDir_Left, 0.2f, NULL, &_dock_mainCentralId);

    ImGui::DockBuilderFinish(dockspace_id);

    _layoutInitialized = true;
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
    // -------------------------------------------------------------------------
    // Window containing the dock layout
    // -------------------------------------------------------------------------

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", 0, flags);
    ImGui::PopStyleVar();

    menuBar();

    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id);

    if (!_layoutInitialized) {
        initializeLayout();
    }

    _imageViewerMutex.lock();

    if (_imageViewer) {
        ImGui::SetNextWindowDockID(_dock_leftId, ImGuiCond_Once);
        ImGui::Begin("Tools");
        _imageViewer->gui();
        ImGui::End();

        ImGui::SetNextWindowDockID(_dock_mainCentralId, ImGuiCond_Once);
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

            // Process scroll events
            if (io.MouseWheel != 0.0f) {
                _imageViewer->mouseScroll(0., io.MouseWheel);
            }
        }
        ImGui::End();
    }

    _imageViewerMutex.unlock();

    // -------------------------------------------------------------------------
    // Finish
    // -------------------------------------------------------------------------

    ImGui::End();
    ImGui::PopStyleVar();

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

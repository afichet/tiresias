#include "App.h"

#include <iostream>
#include <exception>


static void glfw_error_callback(int error, const char *description)
{
    std::cerr << "[ERROR] GLFW error: "
              << error << ": "
              << description << std::endl;
}

int main(int argc, char *argv[])
{
    try {
        App app(argc, argv);

        app.exec();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
find_package(OpenGL REQUIRED)

if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)
endif()


# -----------------------------------------------------------------------------
# GLEW
# -----------------------------------------------------------------------------

if(OPENGL_FOUND AND NOT APPLE)
  list(APPEND GLEW_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c)
  list(APPEND GLEW_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/glew/include/glew.h)

  if( WIN32 )
    list(APPEND GLEW_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/glew/include/wglew.h)
  else()
    list(APPEND GLEW_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/glew/include/glxew.h)
  endif()

  if( WIN32 )
    add_definitions(-DGLEW_STATIC)
    add_definitions(-DGLEW_BUILD)
  endif()
  if(USING_GNU_CXX OR USING_CLANG_CXX)
    # glew.c #includes glew.h which contains "lax" function prototypes
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c PROPERTIES COMPILE_FLAGS  "-Wno-strict-prototypes -Wno-missing-prototypes")
  endif()
endif()


# -----------------------------------------------------------------------------
# GLFW
# -----------------------------------------------------------------------------

if (WIN32)
  list(APPEND GLFW_DEPENDENCIES ${OPENGL_LIBRARIES})
elseif (APPLE)
  find_library(cocoa_library Cocoa)
  find_library(opengl_library OpenGL)
  find_library(corevideo_library CoreVideo)
  find_library(iokit_library IOKit)
  list(APPEND GLFW_DEPENDENCIES ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
  list(APPEND GLFW_DEPENDENCIES GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread )
  if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND GLFW_DEPENDENCIES rt)
  endif()
  if(CMAKE_SYSTEM MATCHES "Linux")
    list(APPEND GLFW_DEPENDENCIES dl)
  endif()
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)


# -----------------------------------------------------------------------------
# ImGUI
# -----------------------------------------------------------------------------

set(IMGUI_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

set(IMGUI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h
)


# -----------------------------------------------------------------------------
# Native File Dialog
# -----------------------------------------------------------------------------
set(NFD_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_common.c  
)

if (WIN32)
  set(NFD_SRCS
    ${NFD_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_win.cpp  
  )
elseif (APPLE)
  set(NFD_SRCS
    ${NFD_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_cocoa.m  
  )
elseif (CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
  # find_package(GTK REQUIRED)
  # find_package(PkgConfig REQUIRED)
  # pkg_check_modules(GTK REQUIRED gtkmm-3.0)
  # message("${GTK_INCLUDE_DIR}")

  find_package(PkgConfig REQUIRED)
  pkg_check_modules (GLIB2 REQUIRED glib-2.0)
  pkg_check_modules(GTK REQUIRED gtk+-3.0)

  set(NFD_SRCS
    ${NFD_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_gtk.c  
  )
endif()

set(NFD_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/simple_exec.h
)

# -----------------------------------------------------------------------------
# LodePNG
# -----------------------------------------------------------------------------

set(LODEPNG_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/lodepng/lodepng.cpp
)

set (LODEPNG_HEADERS
     ${CMAKE_CURRENT_SOURCE_DIR}/lodepng/lodepng.h
)


add_subdirectory(spectral-exr/lib)


# -----------------------------------------------------------------------------
# Library creation
# -----------------------------------------------------------------------------

set(SOURCES ${GLEW_SRCS} ${IMGUI_SRCS} ${NFD_SRCS} ${LODEPNG_SRCS} ImFileDialog/ImFileDialog.cpp)
set(HEADERS ${GLEW_HEADERS} ${IMGUI_HEADERS} ${NFD_HEADERS} ${LODEPNG_HEADERS})

add_library(3rdparty STATIC ${SOURCES})
target_link_libraries(3rdparty PUBLIC glfw ${GLFW_DEPENDENCIES})
target_link_libraries(3rdparty PUBLIC EXRSpectralImage)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/glew/include)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/nativefiledialog/src/include)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lodepng)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/glm)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/ImFileDialog)
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/stb)

# TODO: Temporary: for "spectrum_data.h"
target_include_directories(3rdparty SYSTEM PUBLIC ${CMAKE_CURRENT_LIST_DIR}/spectral-exr/lib)

if (UNIX AND NOT APPLE)
  target_link_libraries(3rdparty PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${X11_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${GLIB2_LIBRARIES}
    ${GTK_LIBRARIES}
  )

  target_include_directories(3rdparty SYSTEM PUBLIC 
    ${GLIB2_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    )
elseif(APPLE)
  target_link_libraries(3rdparty PUBLIC
      "-framework Cocoa"
      "-framework IOKit"
      "-framework CoreVideo"
      "-framework AppKit"
  )
endif()

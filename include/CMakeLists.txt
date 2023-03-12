find_package(glfw3 REQUIRED)

add_library(boilerplate_INC INTERFACE 
    Shader.hpp 
    Window.hpp 
    Window.tpp
    Quad.hpp
    Histogram.hpp
    Image.hpp
    Framebuffer.hpp
    Texture2D.hpp
    Algorithm.hpp
)

target_link_system_libraries(boilerplate_INC INTERFACE glfw::glfw)
target_include_directories(boilerplate_INC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
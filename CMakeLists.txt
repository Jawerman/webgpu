include(FetchContent)

cmake_minimum_required(VERSION 3.0...3.16)
project(
  LearnWebGPU
  VERSION 0.1.0
  LANGUAGES CXX C
)


# Fetchs glfw
if (NOT EMSCRIPTEN)
  FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.4
)
  FetchContent_MakeAvailable(glfw)
else()
  # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
  add_library(glfw INTERFACE)
  target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

# if (NOT EMSCRIPTEN)
#     add_subdirectory(glfw)
# else()
#     # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
#     add_library(glfw INTERFACE)
#     target_link_options(glfw INTERFACE -sUSE_GLFW=3)
# endif()

# if (NOT EMSCRIPTEN)
#     add_subdirectory(glfw)
# else()
#     # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
#     add_library(glfw INTERFACE)
#     target_link_options(glfw INTERFACE -sUSE_GLFW=3)
# endif()


# Include webgpu directory, to define the 'webgpu' target
# add_subdirectory(webgpu)
# Fetchs webgpu distribution
FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
  GIT_TAG main
)
FetchContent_MakeAvailable(webgpu)

# add_executable(
#   App
#   src/main.cpp
#   src/webgpu-utils.h
#   src/webgpu-utils.cpp
# )

FetchContent_Declare(
  glfw3webgpu
  GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu
  GIT_TAG main
)
FetchContent_MakeAvailable(glfw3webgpu)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.hpp")
add_executable(
  App
  ${SOURCES}
)

# Add the 'webgpu' target as a dependency of our App
target_link_libraries(App PRIVATE webgpu glfw glfw3webgpu)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(App)

set_directory_properties(App
  CMAKE_CXX_STANDARD 17
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_EXTENSIONS OFF
  CMAKE_COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
  target_compile_options(App PRIVATE /W4)
elseif (CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
  set_target_properties(App PROPERTIES
    XCODE_GENERATE_SCHEME ON
    XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
  )
endif()

# Options that are specific to Emscripten
if (EMSCRIPTEN)
  # Generate a full web page rather than a simple WebAssembly module
  set_target_properties(App PROPERTIES SUFFIX ".html")
  # Enable the use of emscripten_sleep()
  target_link_options(App PRIVATE -sASYNCIFY)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# generate compile_flags.txt
# "E:\git\cpp\webgpu\build\_deps\glfw-src\include\GLFW\glfw3.h"
set(COMPILE_FLAGS_TXT "${CMAKE_CURRENT_SOURCE_DIR}/src/compile_flags.txt")

set(COMPILE_FLAGS "-I${CMAKE_BINARY_DIR}/_deps/webgpu-backend-wgpu-src/include")
set(COMPILE_FLAGS "${COMPILE_FLAGS}\n-I${CMAKE_BINARY_DIR}/_deps/glfw-src/include")
set(COMPILE_FLAGS "${COMPILE_FLAGS}\n-I${CMAKE_BINARY_DIR}/_deps/glfw3webgpu-src")
# Elimina el archivo si ya existe
if(EXISTS ${COMPILE_FLAGS_TXT})
  file(REMOVE ${COMPILE_FLAGS_TXT})
endif()

file(GENERATE
  OUTPUT ${COMPILE_FLAGS_TXT}
  CONTENT "${COMPILE_FLAGS}"
)
message(STATUS "Generated file: ${COMPILE_FLAGS_TXT} with flags: ${COMPILE_FLAGS_TXT}")

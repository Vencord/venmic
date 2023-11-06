cmake_minimum_required(VERSION 3.16)
project(venmic-addon LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} SHARED)
add_library(vencord::venmic-addon ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-missing-field-initializers -Wno-cast-function-type)

# --------------------------------------------------------------------------------------------------------
# Add source files
# --------------------------------------------------------------------------------------------------------

file(GLOB src "*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("../cmake/cpm.cmake")

CPMFindPackage(
  NAME           range-v3
  GIT_TAG        0.12.0
  GIT_REPOSITORY "https://github.com/ericniebler/range-v3"
)

target_link_libraries(${PROJECT_NAME} PUBLIC vencord::venmic range-v3::meta)

# --------------------------------------------------------------------------------------------------------
# CMake.js related
# --------------------------------------------------------------------------------------------------------

add_definitions(-DNAPI_VERSION=7)

target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_JS_LIB})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_JS_INC})

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

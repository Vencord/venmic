cmake_minimum_required(VERSION 3.16)
project(venmic LANGUAGES CXX VERSION 2.0)

# --------------------------------------------------------------------------------------------------------
# Library options
# --------------------------------------------------------------------------------------------------------

option(venmic_addon         "Build as addon"                            OFF)
option(venmic_server        "Build as rest server"                       ON)
option(venmic_prefer_remote "Prefer remote packages over local packages" ON)

# --------------------------------------------------------------------------------------------------------
# Addon and Rest-Server are mutually exclusive
# --------------------------------------------------------------------------------------------------------

if (venmic_addon)
  set(venmic_server OFF)
endif()

# --------------------------------------------------------------------------------------------------------
# Sync `CPM_DOWNLOAD_ALL` with `venmic_prefer_remote`
# --------------------------------------------------------------------------------------------------------

if (venmic_prefer_remote)
    message(STATUS "[venmic] Avoiding local packages as 'venmic_prefer_remote' is ON")
endif()

set(CPM_DOWNLOAD_ALL ${venmic_prefer_remote})

# --------------------------------------------------------------------------------------------------------
# CMake options
# --------------------------------------------------------------------------------------------------------

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME})
add_library(vencord::venmic ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)

# --------------------------------------------------------------------------------------------------------
# Add source files
# --------------------------------------------------------------------------------------------------------

file(GLOB src "src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})

# --------------------------------------------------------------------------------------------------------
# Include "include" folder
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC  "include")
target_include_directories(${PROJECT_NAME} PRIVATE "include/vencord" "private")

# --------------------------------------------------------------------------------------------------------
# Setup Dependencies
# --------------------------------------------------------------------------------------------------------

include("cmake/cpm.cmake")

CPMFindPackage(
  NAME           rohrkabel
  GIT_TAG        master
  GIT_REPOSITORY "https://github.com/Soundux/rohrkabel"
)

CPMFindPackage(
  NAME           tl-expected
  VERSION        1.1
  GIT_REPOSITORY "https://github.com/TartanLlama/expected"
)

CPMFindPackage(
  NAME           channel
  VERSION        2.1
  GIT_REPOSITORY "https://github.com/Soundux/channel"
)

CPMFindPackage(
  NAME           range-v3
  GIT_TAG        0.12.0
  GIT_REPOSITORY "https://github.com/ericniebler/range-v3"
)

target_link_libraries(${PROJECT_NAME} PUBLIC soundux::rohrkabel tl::expected cr::channel range-v3::meta)

# --------------------------------------------------------------------------------------------------------
# Setup Rest Server
# --------------------------------------------------------------------------------------------------------

if (venmic_server)
  add_subdirectory(server)
endif()

# --------------------------------------------------------------------------------------------------------
# Setup Node Addon
# --------------------------------------------------------------------------------------------------------

if (venmic_addon AND NOT CMAKE_JS_VERSION)
  message(FATAL_ERROR "[venmic] Please build the addon using CMake.js")
endif()

if (venmic_addon)
  add_subdirectory(addon)
endif()
